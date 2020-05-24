#ifndef _ERRORS_H
#define _ERRORS_H

#define PTHREAD_ERROR(func, msg) \
	if((errno = func) != 0) { \
		perror(msg); \
		exit(EXIT_FAILURE); \
	}

#define PTHREAD_CREATE(thread_t, attr, func, attr_2) \
	PTHREAD_ERROR(pthread_create(thread_t, attr, func, attr_2), \
			"Creating " #thread_t " thread")

#define PTHREAD_MUTEX_INIT_ERR(mutex_t, attr) \
	PTHREAD_ERROR(pthread_mutex_init(mutex_t, attr), \
			"Creating " #mutex_t " mutex")

#define PTHREAD_MUTEX_DESTROY_ERR(mutex_t) \
	PTHREAD_ERROR(pthread_mutex_destroy(mutex_t), \
			"Destroying " #mutex_t " mutex")

#define PTHREAD_MUTEX_LOCK(mutex_t) \
	PTHREAD_ERROR(pthread_mutex_lock(mutex_t), "Locking " #mutex_t " mutex")

#define PTHREAD_MUTEX_UNLOCK(mutex_t) \
	PTHREAD_ERROR(pthread_mutex_unlock(mutex_t), "Unlocking " #mutex_t " mutex")

#define PTHREAD_COND_INIT_ERR(cond_t, attr) \
	PTHREAD_ERROR(pthread_cond_init(cond_t, attr), \
			"Creating " #cond_t " condition variable")

#define PTHREAD_COND_DESTROY_ERR(cond_t) \
	PTHREAD_ERROR(pthread_cond_destroy(cond_t), \
			"Destroying " #cond_t " condition variable")

#define PTHREAD_COND_WAIT(cond_t, mutex_t) \
	PTHREAD_ERROR(pthread_cond_wait(cond_t, mutex_t), \
			"Waiting " #cond_t " condition")

#define PTHREAD_COND_SIGNAL(cond_t) \
	PTHREAD_ERROR(pthread_cond_signal(cond_t), \
			"Signaling " #cond_t " condition")

#define PTHREAD_COND_BROADCAST(cond_t) \
	PTHREAD_ERROR(pthread_cond_broadcast(cond_t), \
			"Signaling " #cond_t " condition")

#endif
