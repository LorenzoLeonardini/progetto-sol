#ifndef _READ_WRITE_LOCK_H
#define _READ_WRITE_LOCK_H

typedef struct {
	pthread_mutex_t mtx;
	pthread_cond_t read_go;
	pthread_cond_t write_go;

	int active_readers;
	int active_writers;
	int waiting_readers;
	int waiting_writers;
} rw_lock_struct_t;
typedef rw_lock_struct_t *rw_lock_t;

rw_lock_t rw_lock_create();
void rw_lock_start_read(rw_lock_t rw_lock);
void rw_lock_done_read(rw_lock_t rw_lock);
void rw_lock_start_write(rw_lock_t rw_lock);
void rw_lock_done_write(rw_lock_t rw_lock);
void rw_lock_delete(rw_lock_t rw_lock);

#endif
