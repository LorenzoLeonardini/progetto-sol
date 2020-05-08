#ifndef _COUNTER_H
#define _COUNTER_H

#include <pthread.h>
#include "llds/queue.h"
#include "client.h"

typedef enum { OPEN, CLOSED, CLOSING } status_t;
typedef struct {
	int id;
	queue_t queue;
	pthread_mutex_t mtx;
	status_t status;
} counter_struct_t;
typedef counter_struct_t *counter_t;

counter_t counter_create(int id);
void counter_change_status(counter_t counter, status_t status);
void counter_add_client(counter_t counter, client_t client);
void counter_delete(counter_t counter);

#endif
