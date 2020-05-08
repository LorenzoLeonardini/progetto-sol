#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <signal.h>

#include <pthread.h>

#include "supermarket.h"
#include "consts.h"

static int sigquit = 0, sighup = 0;

static int socket_fd = -1;

static void register_handlers();
static void connect_to_manager();

void supermarket_launch() {
	register_handlers();

	connect_to_manager();

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
	} else if (sighup) {
	}
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
