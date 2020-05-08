#ifndef _ERRORS_H
#define _ERRORS_H

#define PTHREAD_CREATE(name, thread_t, attr, func, attr_2) \
	if((errno = pthread_create(thread_t, attr, func, attr_2)) != 0) { \
		perror("Creating " name " thread"); \
		exit(EXIT_FAILURE); \
	}

#endif
