#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "utils/errors.h"
#include "utils/config.h"
#include "utils/consts.h"
#include "utils/network.h"

#include "counter.h"
#include "guard.h"
#include "logger.h"

#include "supermarket.h"

static int sigquit = 0, sighup = 0;

static counter_t *counters;

static void register_handlers();
static void create_counters();
static void close_connections();

void supermarket_launch() {
	// Signals interrupt system calls in order to stop waits
	register_handlers(FALSE);

	// Init logger
	logger_init("asd.txt");
	// Init counters
	create_counters();

	// Init guard
	pthread_t guard_thread;
	PTHREAD_CREATE(&guard_thread, NULL, &guard_create, NULL);

	// While no signal is received, periodically send the manager info
	// about counters
	while(!sigquit && !sighup) {
		// TODO: send counters data
		sleep(1);
	}
	printf("[Supermarket] received signal, stopping\n");
	fflush(stdout);
	
	// From now on it's better not to be interrupted
	register_handlers(TRUE);

	// TODO: handle
	if(sigquit) {
		guard_close(TRUE);
	} else if (sighup) {
		guard_close(TRUE);
	}
	pthread_join(guard_thread, NULL);
	printf("[Supermarket] All the customers exited the supermarket\n");
	fflush(stdout);

	// Comunicate the manager every customer exited and it can stop listening
	// for connections
	int conn = connect_to_manager_server();
	int data[] = { SO_CLOSE_CONNECTION };
	write(conn, data, sizeof(int));
	close(conn);

	logger_log_general_data(sigquit ? SIGQUIT : SIGHUP);
	logger_cleanup();
	close_connections();
	exit(EXIT_SUCCESS);
}

static void close_connections() {
	// After closing, the file descriptors are reset to -1 to avoid
	// problems in case this function is called twice (or more)
	printf("[Supermarket] Supermarket is shutting down...\n");
	fflush(stdout);
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
	printf("[Supermarket] Cleaning up counters\n");
	fflush(stdout);
	for(int i = 0; i < K; i++) {
		counter_delete(counters[i]);
	}
	free(counters);
}

static void create_counters() {
	counters = (counter_t*) malloc(sizeof(counter_t) * K);
	for(int i = 0; i < K; i++) {
		counters[i] = counter_create(i);
	}
	atexit(free_counters);
}
