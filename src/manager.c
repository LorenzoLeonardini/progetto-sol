#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "utils/consts.h"
#include "utils/errors.h"

#include "manager.h"

static int socket_fd = -1;
static int supermarket_pid = -1;

static void close_connections();
static void register_handlers();
static void start_socket();
static void *fd_to_args(int fd);
static int args_to_fd(void *args);
static void handle_connection(int connection);
static void wait_supermarket_close(int supermarket_pid);

void manager_launch(int sup_pid) {
	supermarket_pid = sup_pid;

	// System Calls should be recovered after interrupt.
	// Especially important for accepting incoming messages.
	register_handlers(TRUE);

	start_socket();
	int s_fd;
	while((s_fd = accept(socket_fd, NULL, 0)) != -1) {
		handle_connection(s_fd);
	}

	// Cleanup sockets
	close_connections();
	
	// The wait shouldn't be interrupted
	register_handlers(TRUE);
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
}

static void gestore(int signum) {
	if(signum == SIGQUIT || signum == SIGHUP) {
		kill(supermarket_pid, signum);
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

static void *fd_to_args(int fd) {
	int *p = (int*) malloc(sizeof(int));
	*p = fd;
	return (void*) p;
}

static int args_to_fd(void *args) {
	pthread_detach(pthread_self());
	int fd = *((int*)args);
	free(args);
	return fd;
}

static void *customer_request_exit(void *args) {
	int connection = args_to_fd(args);
	int customer_id, n_bytes;
	n_bytes = read(connection, &customer_id, sizeof(int));
	if(n_bytes == -1) {
		perror("[Manager] Getting customer id");
		fprintf(stderr, "[Manager] Received comunication from a unidentified customer\n");
		return NULL;
	} else if(n_bytes == 0) {
		fprintf(stderr, "[Manager] Connection with customer ended abruptly\n");
		return NULL;
	}
	int response[2] = { SO_CUSTOMER_GRANT_EXIT, customer_id };
	write(connection, response, sizeof(int) * 2);
	close(connection);
	return NULL;
}

static void handle_connection(int connection) {
	int type = 0, n_bytes = 0;
	n_bytes = read(connection, &type, sizeof(int));
	if(n_bytes == -1) {
		perror("[Manager] Getting connection type");
		fprintf(stderr, "[Manager] Error while trying to get connection type");
		return;
	} else if(n_bytes == 0) {
		fprintf(stderr, "[Manager] Connection with client ended abruptly\n");
	}

	pthread_t thread;
	switch(type) {
		case SO_CLOSE_CONNECTION:
			close(connection);
			close_connections();
			break;
		case SO_CUSTOMER_REQUEST_EXIT:
			PTHREAD_CREATE(&thread, NULL, customer_request_exit, fd_to_args(connection));
			break;
		default:
			fprintf(stderr, "[Manager] Unkown connection type. Killing...\n");
			close(connection);
			break;
	}
}

static void wait_supermarket_close(int supermarket_pid) {
	if(waitpid(supermarket_pid, NULL, 0) == -1) {
		perror("[Manager] Waiting supermarket process to finish");
		exit(EXIT_FAILURE);
	}
}
