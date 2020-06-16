#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "utils/config.h"
#include "utils.h"

#include "manager.h"

static volatile sig_atomic_t signal_received = 0;

static int socket_fd = -1;
static int supermarket_connection = -1;
static int supermarket_pid = -1;

static void start_socket();
static void wait_supermarket_connection();
static void close_connections();
static void wait_supermarket_close(int supermarket_pid);
static void handle_connection(int connection);
static void *customer_request_exit(void *args);
static void *queue_status(void *args);

static void handler(int signum);
static void *fd_to_args(int fd);
static int args_to_fd(void *args);

typedef struct {
	int queue_len;
	msec_t timestamp;
} queue_status_t;

static queue_status_t *queues;
static pthread_mutex_t queues_mtx = PTHREAD_MUTEX_INITIALIZER;

void manager_launch(int sup_pid) {
	supermarket_pid = sup_pid;
	srand(time(NULL));

	// System Calls should be recovered after interrupt. Especially important
	// for accepting incoming messages and waiting supermarket
	register_quit_hup_handlers(TRUE, handler);

	queues = (queue_status_t*) malloc(sizeof(queue_status_t) * K);
	for(int i = 0; i < K; i++) {
		queues[i].timestamp = 0;
		queues[i].queue_len = 0;
	}

	start_socket();
	wait_supermarket_connection();

	int s_fd;
	while((s_fd = accept(socket_fd, NULL, 0)) != -1) {
		handle_connection(s_fd);
	}

	// Cleanup sockets
	close_connections();
	wait_supermarket_close(supermarket_pid);

	PTHREAD_MUTEX_LOCK(&queues_mtx);
	free(queues);
	queues = NULL;
	PTHREAD_MUTEX_UNLOCK(&queues_mtx);

	exit(EXIT_SUCCESS);
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

/**
 * Supermarket connection is needed to communicate counters open/close requests.
 * This function waits until the supermarket connects and updates global
 * variables accordingly.
 */
static void wait_supermarket_connection() {
	while(supermarket_connection == -1) {
		int s_fd = accept(socket_fd, NULL, 0);
		int type;
		MANAGER_SOCKET_READ(s_fd, &type, sizeof(int),
				"[Manager] Getting supermarket connection",
				"Can't get supermarket connection",);
		if(type == SO_SUPERMARKET_CONNECTION)
			supermarket_connection = s_fd;
		else
			close(s_fd);
	}
}

static void close_connections() {
	// After closing, the file descriptors are reset to -1 to avoid
	// problems in case this function is called twice (or more)
	if(socket_fd != -1) {
		MANAGER_LOG("Manager is shutting down...\n");
		close(socket_fd);
		socket_fd = -1;
	}
	if(supermarket_connection != -1) {
		close(supermarket_connection);
		supermarket_connection = -1;
	}
}

static void wait_supermarket_close(int supermarket_pid) {
	int status;
	if(waitpid(supermarket_pid, &status, 0) == -1) {
		perror("[Manager] Waiting supermarket process to finish");
		exit(EXIT_FAILURE);
	}
	if(status != 0) exit(EXIT_FAILURE);
}

static void handle_connection(int connection) {
	int type = 0;
	MANAGER_SOCKET_READ(connection, &type, sizeof(int),
			"[Manager] Getting connection type",
			"Error while trying to get connection type", );

	pthread_t thread;
	switch(type) {
		case SO_CLOSE_CONNECTION:
			close(connection);
			close_connections();
			break;
		case SO_CUSTOMER_REQUEST_EXIT:
			PTHREAD_CREATE(&thread, NULL, customer_request_exit,
				fd_to_args(connection));
			break;
		case SO_COUNTER_QUEUE:
			// Checking if connection should be closed
			PTHREAD_MUTEX_LOCK(&queues_mtx);
			if(signal_received && supermarket_connection != -1) {
				MANAGER_LOG("Connection with supermarket has been closed\n");
				close(supermarket_connection);
				supermarket_connection = -1;
			}
			PTHREAD_MUTEX_UNLOCK(&queues_mtx);
			PTHREAD_CREATE(&thread, NULL, queue_status,
				fd_to_args(connection));
			break;
		default:
			MANAGER_ERROR("Unkown connection type (%d). Killing...\n", type);
			close(connection);
			break;
	}
}

/**
 * Give customer permission to exit the supermarket with no products
 */
static void *customer_request_exit(void *args) {
	block_quit_hup_handlers();
	int connection = args_to_fd(args);
	int customer_id;
	MANAGER_SOCKET_READ(connection, &customer_id, sizeof(int),
			"[Manager] Getting customer id",
			"Received communication from an unidentified customer", NULL);
	MANAGER_LOG("Granting customer %d permission to exit\n", customer_id);
	int response[2] = { SO_CUSTOMER_GRANT_EXIT, customer_id };
	write(connection, response, sizeof(int) * 2);
	close(connection);
	return NULL;
}

/**
 * Handle a queue status info message. Decide if to open or close any and, in
 * case the supermarket is still opened, send the new requirement via socket
 */
static void *queue_status(void *args) {
	block_quit_hup_handlers();
	int connection = args_to_fd(args);
	int data[3];
	msec_t timestamp;
	// Getting current queue status
	MANAGER_SOCKET_READ(connection, data, sizeof(int) * 3,
			"[Manager] Getting open counters count",
			"Received wrong communication about counters status", NULL);
	MANAGER_SOCKET_READ(connection, &timestamp, sizeof(msec_t),
			"[Manager] Getting open counters count",
			"Received wrong communication about counters status", NULL);
	close(connection);

	int counter_number = data[0];
	int counter = data[1];
	int queue = data[2];

	int count_one = 0;
	int count_max = 0;

	PTHREAD_MUTEX_LOCK(&queues_mtx);
	if(queues == NULL) {
		PTHREAD_MUTEX_UNLOCK(&queues_mtx);
		return NULL;
	}
	queues[counter].queue_len = queue;
	queues[counter].timestamp = timestamp;

	// Random log, just for fun and to know things are happening. Synced with
	// the first counter
	if(counter == 0 && supermarket_connection != -1) {
		MANAGER_LOG("Received queues status from counters\n");
		for(int i = 0; i < K; i++)
			if(queues[i].timestamp > timestamp - NOTIFY_TIME)
				printf("\tCounter %d : %d\n", i, queues[i].queue_len);
	}

	// Counting queues with too few and too many customers
	for(int i = 0; i < K; i++) {
		// Old information is ignored
		if(queues[i].timestamp < timestamp - (NOTIFY_TIME / 3))
			continue;
		if(queues[i].queue_len <= 1) count_one++;
		if(queues[i].queue_len >= S2) count_max++;
	}

	// Calculating new counter number
	int to_request = counter_number;
	if(count_one >= S1 && count_one > count_max) to_request--;
	else if(count_max > count_one) to_request++;
	if(to_request < 1) to_request = 1;
	else if(to_request > K) to_request = K;

	if(to_request != counter_number 
			&& queues[counter_number - 1].timestamp > timestamp - NOTIFY_TIME - 100) {
		// Sending request
		if(supermarket_connection != -1) {
			int message[2] = { SO_DESIRED_COUNTERS, to_request };
			int res = write(supermarket_connection, message, sizeof(int) * 2);
			if (res == -1) {
				MANAGER_ERROR("Connection with supermarket closed, no more "
					"control over counter status");
				supermarket_connection = -1;
			}
		}
	}
	PTHREAD_MUTEX_UNLOCK(&queues_mtx);
	return NULL;
}

static void handler(int signum) {
	if(signum == SIGQUIT || signum == SIGHUP) {
		signal_received = 1;
		write(1, "\033[33m[Manager]\033[0m Received signal - forwarding\n", sizeof(char) * 49);
		kill(supermarket_pid, signum);
	}
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
