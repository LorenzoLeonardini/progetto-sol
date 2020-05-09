#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <signal.h>

#include <pthread.h>

#include "utils/consts.h"
#include "utils/errors.h"
#include "utils/config.h"

#include "supermarket.h"
#include "counter.h"
#include "guard.h"
#include "logger.h"

static int sigquit = 0, sighup = 0;

static int socket_fd = -1;

static counter_t *counters;

static void register_handlers();
static void connect_to_manager();
static void create_counters();

void supermarket_launch() {
	logger_init("asd.txt");
	register_handlers();
	connect_to_manager();
	create_counters();
	pthread_t guard_thread;
	PTHREAD_CREATE(&guard_thread, NULL, &guard_create, NULL);

	int command;
	int n_bytes;
	while((n_bytes = read(socket_fd, &command, sizeof(int))) > 0) {
		switch(command) {
			default:
				fprintf(stderr, "[Supermarket] Received unknown "
						"command: %d\n", command);
		}
	}
	if(n_bytes == -1) {
		perror("[Supermarket] Reading from socket");
	}

	// Should already have received the signal, but in case that's not true
	// wait for it
	while(!sigquit && !sighup) {
		sleep(1);
	}

	// TODO: handle
	if(sigquit) {
		guard_close(TRUE);
	} else if (sighup) {
		guard_close(TRUE);
	}
	pthread_join(guard_thread, NULL);
	logger_log_general_data(sigquit ? SIGQUIT : SIGHUP);
	logger_cleanup();
	exit(EXIT_SUCCESS);
}

static void close_connections() {
	// After closing, the file descriptors are reset to -1 to avoid
	// problems in case this function is called twice (or more)
	printf("[Supermarket] Supermarket is shutting down...\n");
	fflush(stdout);
	if(socket_fd != -1) {
		close(socket_fd);
		socket_fd = -1;
	}
}

static void gestore(int signum) {
	if(signum == SIGQUIT)
		sigquit = 1;
	else if(signum == SIGHUP)
		sighup = 1;
}

static void register_handlers() {
	struct sigaction s;
	memset(&s, 0, sizeof(s));
	s.sa_handler = gestore;
	s.sa_flags = SA_RESTART;
	sigaction(SIGQUIT, &s, NULL);
}

static void connect_to_manager() {
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKET_PATH("man_sup"), UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	int max_attempts = 10;
	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd == -1) {
		perror("[Supermarket] Connecting to manager socket");
		exit(EXIT_FAILURE);
	}
	while(connect(socket_fd, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
		if(errno == ENOENT) {
			if(!max_attempts--) {
				fprintf(stderr, "Connection timed out\n");
				exit(EXIT_FAILURE);
			}
			sleep(1);
		} else {
			fprintf(stderr, "Can't connect to manager socket\n");
			exit(EXIT_FAILURE);
		}
	}
	atexit(close_connections);
}

static void free_counters() {
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
