#ifndef _NETWORK_UTILS_H
#define _NETWORK_UTILS_H

#define SO_SUPERMARKET_CONNECTION 1
#define SO_CLOSE_CONNECTION 2
#define SO_COUNTER_QUEUE 3
#define SO_DESIRED_COUNTERS 4
#define SO_CUSTOMER_REQUEST_EXIT 5
#define SO_CUSTOMER_GRANT_EXIT 6

#define MANAGER_SOCKET_READ(sock, point, size, perror_msg, error, ret) { \
	int n_bytes; \
	do { \
		n_bytes = read(sock, point, size); \
	} while(n_bytes == -1 && errno == EINTR); \
	if(n_bytes == -1) { \
		perror(perror_msg); \
		MANAGER_ERROR(error "\n"); \
		return ret; \
	} else if(n_bytes == 0) { \
		MANAGER_ERROR("Connection with supermarket ended abruptly\n"); \
		return ret; \
	}}

int connect_to_manager_server();

#endif
