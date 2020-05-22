#ifndef _NETWORK_UTILS_H
#define _NETWORK_UTILS_H

#define MANAGER_SOCKET_READ(sock, point, size, perror_msg, error, ret) { \
	int n_bytes = read(sock, point, size); \
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
