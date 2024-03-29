#ifndef _UTILS_H
#define _UTILS_H

#include "llds/errors.h"
#include "utils/network.h"
#include "utils/signals.h"
#include "utils/time.h"

#define UNIX_PATH_MAX 108
#define SOCKET_PATH(x) "./sockets/" x

#define TRUE 1
#define FALSE 0

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

#endif
