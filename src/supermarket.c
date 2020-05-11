#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "llds/read_write_lock.h"
#include "utils/errors.h"
#include "utils/config.h"
#include "utils/consts.h"
#include "utils/network.h"

#include "counter.h"
#include "guard.h"
#include "logger.h"

#include "supermarket.h"

static int sigquit = 0, sighup = 0;

counter_t *counters;
int opened_counters;

rw_lock_t counters_status = NULL;

static void register_handlers();
static void create_counters();
static void open_counter();
static void close_counter();
static void close_connections();

void supermarket_launch() {
	// Signals interrupt system calls in order to stop waits
	register_handlers(FALSE);

	// Init logger
	logger_init("asd.txt");
	// Init counters
	create_counters();
	opened_counters = 0;
	open_counter();

	// Init guard
	pthread_t guard_thread;
	PTHREAD_CREATE(&guard_thread, NULL, &guard_create, NULL);

	// While no signal is received, periodically send the manager info
	// about counters
	while(!sigquit && !sighup) {
		sleep(3);
		
		// Send the manager the current queue count for every counter
		// Retrieve the queue lengths and construct the message
		// We don't need the read lock, since this is the only thread
		// capable of writing
		int length = sizeof(int) * (opened_counters + 2);
		int *message = (int*) malloc(length);
		message[0] = SO_COUNTER_QUEUE;
		message[1] = opened_counters;
		for(int i = 0; i < opened_counters; i++) {
			message[2 + i] = counter_queue_length(counters[i]);
		}
		// Send the data and get a response
		int connection = connect_to_manager_server();
		write(connection, message, length);
		read(connection, message, sizeof(int) * 2);
		close(connection);
		// Process the response
		assert(message[0] == SO_DESIRED_COUNTERS);
		int desired_counters = message[1];
		free(message);

		SUPERMARKET_LOG("Manager requested %d counters\n", desired_counters);

		if(desired_counters != opened_counters) {
			rw_lock_start_write(counters_status);
			while(opened_counters < desired_counters)
				open_counter();
			while(opened_counters > desired_counters)
				close_counter();
			rw_lock_done_write(counters_status);
		}
	}
	SUPERMARKET_LOG("Received signal, stopping\n");
	
	// From now on it's better not to be interrupted
	register_handlers(TRUE);

	// TODO: handle
	if(sigquit) {
		guard_close(TRUE);
	} else if (sighup) {
		guard_close(TRUE);
	}
	pthread_join(guard_thread, NULL);
	SUPERMARKET_LOG("All the customers exited the supermarket\n");
	SUPERMARKET_LOG("Closing all counters\n");
	while(opened_counters > 0)
		close_counter();
	for(int i = 0; i < K; i++) {
		counter_t closing = counters[i];
		PTHREAD_MUTEX_LOCK(&closing->mtx);
		while(closing->status != CLOSED) {
			PTHREAD_COND_SIGNAL(&closing->idle);
			PTHREAD_COND_WAIT(&closing->idle, &closing->mtx);
		}
		closing->status = GO_HOME;
		PTHREAD_COND_SIGNAL(&closing->idle);
		while(closing->status != WENT_HOME)
			PTHREAD_COND_WAIT(&closing->idle, &closing->mtx);
		PTHREAD_MUTEX_UNLOCK(&closing->mtx);
	}
	SUPERMARKET_LOG("All counters closed\n");

	// Comunicate the manager every customer exited and it can stop listening
	// for connections
	int conn = connect_to_manager_server();
	int data[] = { SO_CLOSE_CONNECTION };
	write(conn, data, sizeof(int));
	close(conn);

	logger_log_general_data(sigquit ? SIGQUIT : SIGHUP);
	for(int i = 0; i < K; i++)
		logger_log_counter_data(counters[i]);
	logger_cleanup();
	close_connections();
	exit(EXIT_SUCCESS);
}

static void close_connections() {
	// After closing, the file descriptors are reset to -1 to avoid
	// problems in case this function is called twice (or more)
	SUPERMARKET_LOG("Supermarket is shutting down...\n");
}

static void gestore(int signum) {
	if(signum == SIGQUIT)
		sigquit = 1;
	else if(signum == SIGHUP)
		sighup = 1;
}

static void register_handlers(int restart) {
	struct sigaction s;
	memset(&s, 0, sizeof(s));
	s.sa_handler = gestore;
	if(restart) {
		s.sa_flags = SA_RESTART;
	}
	sigaction(SIGQUIT, &s, NULL);
}

static void free_counters() {
	SUPERMARKET_LOG("Cleaning up counters\n");
	for(int i = 0; i < K; i++) {
		counter_delete(counters[i]);
	}
	free(counters);

	rw_lock_delete(counters_status);
}

static void create_counters() {
	counters_status = rw_lock_create();

	pthread_t thread;
	counters = (counter_t*) malloc(sizeof(counter_t) * K);
	for(int i = 0; i < K; i++) {
		counters[i] = counter_create(i);
		PTHREAD_CREATE(&thread, NULL, counter_thread_fnc, counters[i]);
	}
	atexit(free_counters);
}

static void open_counter() {
	counter_open(counters[opened_counters]);
	PTHREAD_COND_SIGNAL(&counters[opened_counters]->idle);
	opened_counters++;
}

static void close_counter() {
	opened_counters--;
	SUPERMARKET_LOG("Closing counter %d\n", opened_counters);
	counter_t closing = counters[opened_counters];
	PTHREAD_MUTEX_LOCK(&closing->mtx);
	closing->status = CLOSING;
	PTHREAD_COND_SIGNAL(&closing->idle);
	PTHREAD_MUTEX_UNLOCK(&closing->mtx);
}
