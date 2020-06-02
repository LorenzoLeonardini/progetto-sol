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
#include "utils/config.h"
#include "utils.h"

#include "counter.h"
#include "guard.h"
#include "logger.h"

#include "supermarket.h"

static volatile sig_atomic_t sigquit = 0, sighup = 0;

counter_t *counters;
int opened_counters;
int supermarket_opened = TRUE;

rw_lock_t counters_status = NULL;

static void create_counters();
static void open_counter();
static void supermarket_loop(int connection);
static void close_counter();
static void free_counters();

static void handler(int signum);

void supermarket_launch() {
	// Signals interrupt system calls in order to stop while
	register_quit_hup_handlers(FALSE, handler);

	srand(time(NULL));

	// Init logger
	logger_init(LOG_FILE);
	// Init counters
	create_counters();
	opened_counters = 0;
	for(int i = 0; i < INITIAL_K; i++) {
		open_counter();
	}

	// Connect to manager, and prepare for listening for counters instructions
	int conn = connect_to_manager_server();
	int msg = SO_SUPERMARKET_CONNECTION;
	write(conn, &msg, sizeof(int));

	// Init and start guard
	pthread_t guard_thread;
	PTHREAD_CREATE(&guard_thread, NULL, &guard_create, NULL);

	// Listen for counters instructions
	supermarket_loop(conn);
	SUPERMARKET_LOG("Received signal, stopping\n");

	if(sigquit) {
		rw_lock_start_write(counters_status);
		supermarket_opened = FALSE;
		rw_lock_stop_write(counters_status);
		guard_close_entrance();
		while(opened_counters > 0)
			close_counter();
		guard_close(FALSE);
	} else if (sighup) {
		guard_close(TRUE);
	}

	// From now on it's better not to be interrupted
	register_quit_hup_handlers(TRUE, handler);
	pthread_join(guard_thread, NULL);
	SUPERMARKET_LOG("All the customers exited the supermarket\n");
	SUPERMARKET_LOG("Closing all counters\n");
	while(opened_counters > 0)
		close_counter();
	SUPERMARKET_LOG("All counters closed\n");

	// Communicate the manager every customer exited and it can stop listening
	// for connections
	int conn2 = connect_to_manager_server();
	int data[] = { SO_CLOSE_CONNECTION };
	write(conn2, data, sizeof(int));
	close(conn2);
	close(conn);

	// Log
	logger_log_general_data(sigquit ? SIGQUIT : SIGHUP);
	for(int i = 0; i < K; i++)
		logger_log_counter_data(counters[i]);
	logger_cleanup();

	free_counters();

	SUPERMARKET_LOG("Supermarket is shutting down...\n");
	fflush(stdout);
	exit(EXIT_SUCCESS);
}

static void create_counters() {
	counters_status = rw_lock_create();
	counters = (counter_t*) malloc(sizeof(counter_t) * K);
	for(int i = 0; i < K; i++) {
		counters[i] = counter_create(i);
	}
	atexit(free_counters);
}

static void open_counter() {
	rw_lock_start_write(counters_status);
	opened_counters++;
	PTHREAD_CREATE(&counters[opened_counters - 1]->thread, NULL,
			counter_thread_fnc, counters[opened_counters - 1]);
}

static void supermarket_loop(int connection) {
	int n_bytes;
	int message[2];
	while(!sighup && !sigquit && 
			(n_bytes = read(connection, &message, sizeof(int) * 2)) > 0) {
		if(message[0] != SO_DESIRED_COUNTERS) {
			// Not necessarily a problem, it could just be the signal
			SUPERMARKET_ERROR("Expected SO_DESIRED_COUNTERS, not received.\n");
			exit(EXIT_FAILURE);
		}
		int desired_counters = message[1];

		SUPERMARKET_LOG("Manager requested %d counters\n", desired_counters);

		if(desired_counters != opened_counters) {
			while(opened_counters < desired_counters)
				open_counter();
			while(opened_counters > desired_counters)
				close_counter();
		}
	}
}

static void close_counter() {
	rw_lock_start_write(counters_status);
	opened_counters--;
	SUPERMARKET_LOG("Closing counter %d\n", opened_counters);
	counter_t closing = counters[opened_counters];

	PTHREAD_MUTEX_LOCK(&closing->mtx);
	closing->status = CLOSING;
	PTHREAD_COND_SIGNAL(&closing->idle);
	PTHREAD_MUTEX_UNLOCK(&closing->mtx);
	rw_lock_stop_write(counters_status);

	pthread_join(closing->thread, NULL);
}

static void free_counters() {
	if(counters == NULL) return;
	SUPERMARKET_LOG("Cleaning up counters\n");
	for(int i = 0; i < K; i++) {
		counter_destroy(counters[i]);
	}
	free(counters);
	counters = NULL;

	rw_lock_destroy(counters_status);
}

static void handler(int signum) {
	if(signum == SIGQUIT)
		sigquit = 1;
	else if(signum == SIGHUP)
		sighup = 1;
}
