#ifndef _CONSTS_H
#define _CONSTS_H

#define UNIX_PATH_MAX 108
#define SOCKET_PATH(x) "./sockets/" x

#define MANAGER_LOG(...) \
	printf("\033[33m[Manager]\033[0m " __VA_ARGS__); \
	fflush(stdout)
#define MANAGER_ERROR(...) \
	fprintf(stderr, "\033[31m[Manager]\033[0m " __VA_ARGS__)

#define SUPERMARKET_LOG(...) \
	printf("\033[32m[Supermarket]\033[0m " __VA_ARGS__); \
	fflush(stdout)
#define SUPERMARKET_ERROR(...) \
	fprintf(stderr, "\033[31m[Supermarket]\033[0m " __VA_ARGS__)

#define TRUE 1
#define FALSE 0

#define SO_CLOSE_COUNTER 1
#define SO_OPEN_COUNTER 2
#define SO_CLOSE_CONNECTION 3
#define SO_COUNTER_QUEUE 4
#define SO_CUSTOMER_REQUEST_EXIT 5
#define SO_CUSTOMER_GRANT_EXIT 6
#define SO_SUPERMARKET_CONNECTION 7
#define SO_DESIRED_COUNTERS 8

#endif
