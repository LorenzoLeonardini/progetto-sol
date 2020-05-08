#ifndef _ERRORS_H
#define _ERRORS_H

#define PTHREAD_CREATE(thread_t, attr, func, attr_2) \
	if((errno = pthread_create(thread_t, attr, func, attr_2)) != 0) { \
		perror("Creating " #thread_t " thread"); \
		exit(EXIT_FAILURE); \
	}

#define PTHREAD_MUTEX_INIT_ERR(mutex_t, attr) \
	if((errno = pthread_mutex_init(mutex_t, attr)) != 0) { \
		perror("Creating " #mutex_t " mutex"); \
		exit(EXIT_FAILURE); \
	}

#define PTHREAD_MUTEX_DESTROY_ERR(mutex_t) \
	if((errno = pthread_mutex_destroy(mutex_t)) != 0) { \
		perror("Creating " #mutex_t " mutex"); \
		exit(EXIT_FAILURE); \
	}

#define PTHREAD_MUTEX_LOCK(mutex_t) \
	if((errno = pthread_mutex_lock(mutex_t)) != 0) { \
		perror("Locking " #mutex_t " mutex"); \
		exit(EXIT_FAILURE); \
	}

#define PTHREAD_MUTEX_UNLOCK(mutex_t) \
	if((errno = pthread_mutex_unlock(mutex_t)) != 0) { \
		perror("Locking " #mutex_t " mutex"); \
		exit(EXIT_FAILURE); \
	}

#define PTHREAD_COND_WAIT(cond_t, mutex_t) \
	if((errno = pthread_cond_wait(cond_t, mutex_t)) != 0) { \
		perror("Waiting " #cond_t " condition"); \
		exit(EXIT_FAILURE); \
	}

#define PTHREAD_COND_SIGNAL(cond_t) \
	if((errno = pthread_cond_signal(cond_t)) != 0) { \
		perror("Signaling " #cond_t " condition"); \
		exit(EXIT_FAILURE); \
	}

#endif
