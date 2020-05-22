#ifndef _UTILS_H
#define _UTILS_H

#include "llds/errors.h"
#include "utils/consts.h"
#include "utils/network.h"
#include "utils/time.h"

#ifndef SILENT
#define MANAGER_LOG(...) \
	printf("\033[33m[Manager]\033[0m " __VA_ARGS__); \
	fflush(stdout)
#define SUPERMARKET_LOG(...) \
	printf("\033[32m[Supermarket]\033[0m " __VA_ARGS__); \
	fflush(stdout)
#else
#define MANAGER_LOG(...)
#define SUPERMARKET_LOG(...)
#endif

#define MANAGER_ERROR(...) \
	fprintf(stderr, "\033[31m[Manager]\033[0m " __VA_ARGS__)
#define SUPERMARKET_ERROR(...) \
	fprintf(stderr, "\033[31m[Supermarket]\033[0m " __VA_ARGS__)


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

#endif
