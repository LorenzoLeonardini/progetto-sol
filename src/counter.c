#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <errno.h>

#include "utils/errors.h"
#include "customer.h"

#include "counter.h"

counter_t counter_create(int id) {
	counter_t counter = (counter_t) malloc(sizeof(counter_struct_t));
	counter->id = id;
	counter->queue = queue_create();
	counter->status = OPEN;
	PTHREAD_MUTEX_INIT_ERR(&counter->mtx, NULL);
	return counter;
}

void counter_change_status(counter_t counter, status_t status) {
	PTHREAD_MUTEX_LOCK(&counter->mtx);
	counter->status = status;
	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
}

void counter_add_customer(counter_t counter, customer_t customer) {
	PTHREAD_MUTEX_LOCK(&counter->mtx);

	if(counter->status != OPEN) {
		fprintf(stderr, "Trying to add a client to a closed counter\n");
	} else {
		queue_add(counter->queue, customer);
	}
	
	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
}

void counter_delete(counter_t counter) {
	PTHREAD_MUTEX_DESTROY_ERR(&counter->mtx);
	queue_delete(counter->queue);
	free(counter);
}
