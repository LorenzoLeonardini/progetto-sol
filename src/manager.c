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
#include <sys/wait.h>

#include "utils/config.h"
#include "utils.h"

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
	srand(time(NULL));

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
		MANAGER_LOG("Manager is shutting down...\n");
		close(socket_fd);
		socket_fd = -1;
	}
}

static void gestore(int signum) {
	if(signum == SIGQUIT || signum == SIGHUP) {
//		write(0, "\t---- Manager received signal - forwarding ----\n", sizeof(char) * 49);
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
	sigaction(SIGHUP, &s, NULL);
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
	int customer_id;
	MANAGER_SOCKET_READ(connection, &customer_id, sizeof(int),
			"[Manager] Getting customer id",
			"Received communication from an unidentified customer", NULL)
	MANAGER_LOG("Granting customer %d permission to exit\n", customer_id);
	int response[2] = { SO_CUSTOMER_GRANT_EXIT, customer_id };
	write(connection, response, sizeof(int) * 2);
	close(connection);
	return NULL;
}

static void queue_status(int connection) {
	int counters_count;
	MANAGER_SOCKET_READ(connection, &counters_count, sizeof(int),
			"[Manager] Getting open counters count", 
			"Received wrong comunication about counters status", );
	assert(counters_count > 0);
	int *queues = (int*) malloc(sizeof(int) * counters_count);
	MANAGER_SOCKET_READ(connection, queues, sizeof(int) * counters_count,
			"[Manager] Getting open counters count",
			"Received wrong comunication about counters status", );
	MANAGER_LOG("Received queues status from supermarket\n");
	int count_one = 0;
	int count_max = 0;
	for(int i = 0; i < counters_count; i++) {
		if(queues[i] <= 1) count_one++;
		if(queues[i] >= S2) count_max++;
		printf("\tCounter %d: %d customers\n", i, queues[i]);
	}
	int to_request = counters_count;
	if(count_one >= S1 && count_max == 0) to_request--;
	else if(count_one < S1 && count_max > 0) to_request++;
	if(to_request < 1) to_request = 1;
	else if(to_request > K) to_request = K;
	int message[2] = { SO_DESIRED_COUNTERS, to_request };
	write(connection, message, sizeof(int) * 2);
	free(queues);
}

static void *supermarket_connection(void *args) {
	int connection = args_to_fd(args);
	int bytes, message;
	while((bytes = read(connection, &message, sizeof(int))) > 0) {
		if(message == SO_COUNTER_QUEUE) {
			queue_status(connection);
		} else {
			MANAGER_ERROR("Received undefined message via supermarket socket");
		}
	}
	MANAGER_LOG("Closed connection with supermarket\n");
	return NULL;
}

static void handle_connection(int connection) {
	int type = 0;
	MANAGER_SOCKET_READ(connection, &type, sizeof(int),
			"[Manager] Getting connection type",
			"Error while trying to get connection type", )

	pthread_t thread;
	switch(type) {
		case SO_CLOSE_CONNECTION:
			close(connection);
			close_connections();
			break;
		case SO_CUSTOMER_REQUEST_EXIT:
			PTHREAD_CREATE(&thread, NULL, customer_request_exit, fd_to_args(connection));
			break;
		case SO_SUPERMARKET_CONNECTION:
			PTHREAD_CREATE(&thread, NULL, supermarket_connection, fd_to_args(connection));
			break;
		default:
			MANAGER_ERROR("Unkown connection type (%d). Killing...\n", type);
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
