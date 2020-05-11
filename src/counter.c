#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "llds/queue.h"
#include "utils/consts.h"
#include "utils/errors.h"
#include "customer.h"

#include "counter.h"

static void counter_close(counter_t counter);

counter_t counter_create(int id) {
	counter_t counter = (counter_t) malloc(sizeof(counter_struct_t));
	counter->id = id;
	counter->queue = queue_create();
	counter->status = CLOSED;
	counter->tot_customers = 0;
	counter->tot_products = 0;
	counter->open_count = 0;
	counter->open_timestamp = 0;
	counter->open_time = queue_create();
	PTHREAD_MUTEX_INIT_ERR(&counter->mtx, NULL);
	PTHREAD_COND_INIT_ERR(&counter->idle, NULL);
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
		SUPERMARKET_ERROR("Trying to add a client to a closed counter\n");
	} else {
		queue_add(counter->queue, customer);
	}
	
	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
}

void *counter_thread_fnc(void *args) {
	pthread_detach(pthread_self());
	counter_t counter = (counter_t) args;
	PTHREAD_MUTEX_LOCK(&counter->mtx);
	while(counter->status != GO_HOME) {
		while(counter->status == OPEN || counter->status == CLOSED) {
			PTHREAD_COND_WAIT(&counter->idle, &counter->mtx);
		}
		if(counter->status == CLOSING) {
			counter_close(counter);
			PTHREAD_COND_SIGNAL(&counter->idle);
		}
	}
	counter->status = WENT_HOME;
	PTHREAD_COND_SIGNAL(&counter->idle);
	SUPERMARKET_LOG("Cashier %d went home\n", counter->id);
	fflush(stdout);
	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
	return NULL;
}

int counter_queue_length(counter_t counter) {
	PTHREAD_MUTEX_LOCK(&counter->mtx);
	int value = counter->queue->size;
	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
	return value;
}

void counter_open(counter_t counter) {
	PTHREAD_MUTEX_LOCK(&counter->mtx);

	// Get opening time
	struct timeval start;
	gettimeofday(&start, NULL);
	unsigned long long millis = (start.tv_sec * 1000000 + start.tv_usec) / 1000;
	counter->status = OPEN;
	counter->open_timestamp = millis;

	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
}

static void counter_close(counter_t counter) {
	// Get close time
	struct timeval stop;
	gettimeofday(&stop, NULL);
	unsigned long long millis = (stop.tv_sec * 1000000 + stop.tv_usec) / 1000;
	counter->open_count++;
	// Calculate open time and save into list
	unsigned long long *t = (unsigned long long*) malloc(sizeof(unsigned long long));
	*t = millis - counter->open_timestamp;
	queue_add(counter->open_time, (void*) t);
	// Change status
	counter->status = CLOSED;
	counter->open_timestamp = 0;

	customer_t customer;
	while((customer = queue_pop(counter->queue)) != NULL) {
		customer->current_queue = -1;
		// TODO: signal the customer
	}

	SUPERMARKET_LOG("Counter %d has been closed\n", counter->id);
}

void counter_delete(counter_t counter) {
	PTHREAD_MUTEX_DESTROY_ERR(&counter->mtx);
	PTHREAD_COND_DESTROY_ERR(&counter->idle);
	queue_delete(counter->queue);
	queue_delete(counter->open_time);
	if(counter->open_timestamp != 0)
		SUPERMARKET_ERROR("Counter %d hasn't been saved\n", counter->id);
	free(counter);
}
