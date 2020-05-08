#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <signal.h>

#include <pthread.h>

#include "manager.h"
#include "consts.h"

static int sigquit = 0, sighup = 0;

static int socket_fd = -1, supermarket_connection = -1;

static void close_connections();
static void register_handlers();
static void start_socket();
static void wait_supermarket_connection();
static void wait_supermarket_close(int supermarket_pid);

void manager_launch(int supermarket_pid) {
	register_handlers(FALSE);

	start_socket();
	wait_supermarket_connection();

	int command;
	int n_bytes;
	while((n_bytes = read(supermarket_connection, &command, sizeof(int))) > 0) {
		switch(command) {
			default:
				fprintf(stderr, "[Manager] Received unknown "
						"command: %d\n", command);
		}
	}

	register_handlers(TRUE);
	while(!sigquit && !sighup) {
		sleep(1);
	}
	if(sigquit) {
		kill(supermarket_pid, SIGQUIT);
	} else if(sighup) {
		kill(supermarket_pid, SIGHUP);
	}
	// Close connections
	close_connections();
	
	wait_supermarket_close(supermarket_pid);
	exit(EXIT_SUCCESS);
}

static void close_connections() {
	// After closing, the file descriptors are reset to -1 to avoid
	// problems in case this function is called twice (or more)
	if(socket_fd != -1) {
		printf("[Manager] Manager is shutting down...\n");
		fflush(stdout);
		close(socket_fd);
		socket_fd = -1;
	}
	if(supermarket_connection != -1) {
		close(supermarket_connection);
		supermarket_connection = -1;
	}
}

static void gestore(int signum) {
	if(signum == SIGQUIT) {
		sigquit = 1;
	} else if(signum == SIGHUP) {
		sighup = 1;
	}
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

static void start_socket() {
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKET_PATH("man_sup"), UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd == -1) {
		perror("[Manager] Starting manager socket");
		exit(EXIT_FAILURE);
	}
	unlink(SOCKET_PATH("man_sup"));
	bind(socket_fd, (struct sockaddr*) &sa, sizeof(sa));
	listen(socket_fd, SOMAXCONN);
	atexit(close_connections);
}

static void wait_supermarket_connection() {
	while((supermarket_connection = accept(socket_fd, NULL, 0)) == -1) {
		perror("[Manager] Waiting the supermarket connection");
	}
}

static void wait_supermarket_close(int supermarket_pid) {
	if(waitpid(supermarket_pid, NULL, 0) == -1) {
		perror("[Manager] Waiting supermarket process to finish");
		exit(EXIT_FAILURE);
	}
}
