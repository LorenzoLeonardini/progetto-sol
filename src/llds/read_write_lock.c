#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>

// TODO: shouldn't use from main project
#include "../utils/errors.h"

#include "read_write_lock.h"

static int rw_lock_read_should_wait(rw_lock_t rw_lock);
static int rw_lock_write_should_wait(rw_lock_t rw_lock);

rw_lock_t rw_lock_create() {
	rw_lock_t rw_lock = (rw_lock_t) malloc(sizeof(rw_lock_struct_t));
	
	rw_lock->active_readers = 0;
	rw_lock->active_writers = 0;
	rw_lock->waiting_readers = 0;
	rw_lock->waiting_writers = 0;

	PTHREAD_MUTEX_INIT_ERR(&rw_lock->mtx, NULL);
	PTHREAD_COND_INIT_ERR(&rw_lock->read_go, NULL);
	PTHREAD_COND_INIT_ERR(&rw_lock->write_go, NULL);

	return rw_lock;
}

void rw_lock_start_read(rw_lock_t rw_lock) {
	PTHREAD_MUTEX_LOCK(&rw_lock->mtx);

	rw_lock->waiting_readers++;
	while(rw_lock_read_should_wait(rw_lock)) {
		PTHREAD_COND_WAIT(&rw_lock->read_go, &rw_lock->mtx);
	}
	rw_lock->waiting_readers--;
	rw_lock->active_readers++;
	
	PTHREAD_MUTEX_UNLOCK(&rw_lock->mtx);
}

void rw_lock_done_read(rw_lock_t rw_lock) {
	PTHREAD_MUTEX_LOCK(&rw_lock->mtx);

	rw_lock->active_readers--;
	if(rw_lock->active_readers == 0 && rw_lock->waiting_writers > 0) {
		PTHREAD_COND_SIGNAL(&rw_lock->write_go);
	}
	
	PTHREAD_MUTEX_UNLOCK(&rw_lock->mtx);
}

void rw_lock_start_write(rw_lock_t rw_lock) {
	PTHREAD_MUTEX_LOCK(&rw_lock->mtx);

	rw_lock->waiting_writers++;
	while(rw_lock_write_should_wait(rw_lock)) {
		PTHREAD_COND_WAIT(&rw_lock->write_go, &rw_lock->mtx);
	}
	rw_lock->waiting_writers--;
	rw_lock->active_writers++;

	PTHREAD_MUTEX_UNLOCK(&rw_lock->mtx);
}

void rw_lock_done_write(rw_lock_t rw_lock) {
	PTHREAD_MUTEX_LOCK(&rw_lock->mtx);

	rw_lock->active_writers--;
	if(rw_lock->waiting_writers > 0) {
		PTHREAD_COND_SIGNAL(&rw_lock->write_go);
	} else if(rw_lock->waiting_readers > 0) {
		PTHREAD_COND_SIGNAL(&rw_lock->read_go);
	}

	PTHREAD_MUTEX_UNLOCK(&rw_lock->mtx);
}

void rw_lock_delete(rw_lock_t rw_lock) {
	PTHREAD_MUTEX_DESTROY_ERR(&rw_lock->mtx);
	PTHREAD_COND_DESTROY_ERR(&rw_lock->read_go);
	PTHREAD_COND_DESTROY_ERR(&rw_lock->write_go);

	free(rw_lock);
}

static int rw_lock_read_should_wait(rw_lock_t rw_lock) {
	if(rw_lock->active_writers > 0 || rw_lock->active_writers > 0) {
		return 1;
	}
	return 0;
}

static int rw_lock_write_should_wait(rw_lock_t rw_lock) {
	if(rw_lock->active_readers > 0 || rw_lock->active_writers > 0) {
		return 1;
	}
	return 0;
}

