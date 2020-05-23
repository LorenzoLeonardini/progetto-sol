#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "llds/queue.h"
#include "utils.h"
#include "utils/config.h"
#include "customer.h"

#include "counter.h"

static void counter_close(counter_t counter);

counter_t counter_create(unsigned int id) {
	counter_t counter = (counter_t) malloc(sizeof(counter_struct_t));
	counter->id = id;
	counter->queue = queue_create();
	counter->status = CLOSED;

	counter->time_for_customer = rand() % 61 + 20;

	counter->tot_customers = 0;
	counter->tot_products = 0;

	counter->opening_count = 0;
	counter->open_timestamp = 0;
	counter->open_time = queue_create();
	counter->client_time = queue_create();

	PTHREAD_MUTEX_INIT_ERR(&counter->mtx, NULL);
	PTHREAD_COND_INIT_ERR(&counter->idle, NULL);
	return counter;
}

void counter_add_customer(counter_t counter, customer_t customer) {
	PTHREAD_MUTEX_LOCK(&counter->mtx);

	if(counter->status != OPEN) {
		SUPERMARKET_ERROR("Trying to add a client to a closed counter\n");
	} else {
		queue_enqueue(counter->queue, customer);
	}

	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
}

void *counter_thread_fnc(void *args) {
	block_quit_hup_handlers();
	counter_t counter = (counter_t) args;
	PTHREAD_MUTEX_LOCK(&counter->mtx);
	counter->open_timestamp = current_time_millis();
	counter->status = OPEN;
	while(counter->status != CLOSED) {
		while(counter->status == OPEN && counter->queue->size == 0) {
			PTHREAD_COND_WAIT(&counter->idle, &counter->mtx);
		}
		if(counter->status == CLOSING) {
			counter_close(counter);
			PTHREAD_COND_SIGNAL(&counter->idle);
		}
		if(counter->status == OPEN) {
			customer_t current = queue_dequeue(counter->queue);
			PTHREAD_MUTEX_UNLOCK(&counter->mtx);
			PTHREAD_MUTEX_LOCK(&current->mtx);
			msec_t start = current_time_millis();
			current->being_served = 1;
			current->queue_time = current_time_millis() - current->queue_time;
			int prods = current->products;
			PTHREAD_MUTEX_UNLOCK(&current->mtx);
			counter->tot_customers++;
			counter->tot_products += prods;

			struct timespec tim = millis_to_timespec(
					counter->time_for_customer + prods * PRODUCT_TIME);
			int ret = nanosleep(&tim, &tim);
			if(ret == -1) {
				if(errno != EINTR) {
					perror("Customer paying");
					SUPERMARKET_ERROR("Counter [%u] haven't waited %lu.%lu and "
							"wasn't stopped by signal\n", counter->id, tim.tv_sec,
							tim.tv_nsec);
					exit(EXIT_FAILURE);
				}
			}

			PTHREAD_MUTEX_LOCK(&current->mtx);
			current->served = 1;
			current->current_queue = -1;
			msec_t *t = (msec_t*) malloc(sizeof(msec_t));
			*t = current_time_millis() - start;
			queue_enqueue(counter->client_time, t);

			PTHREAD_COND_SIGNAL(&current->waiting_in_line);
			PTHREAD_MUTEX_UNLOCK(&current->mtx);
			PTHREAD_MUTEX_LOCK(&counter->mtx);
		}
	}
	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
	return NULL;
}

unsigned int counter_queue_length(counter_t counter) {
	PTHREAD_MUTEX_LOCK(&counter->mtx);
	unsigned int value = counter->queue->size;
	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
	return value;
}

// It's a local function, the lock is already acquired by the caller
static void counter_close(counter_t counter) {
	// Save open time details
	counter->opening_count++;
	msec_t *t = (msec_t*) malloc(sizeof(msec_t));
	*t = current_time_millis() - counter->open_timestamp;
	queue_enqueue(counter->open_time, t);

	// Change status
	counter->status = CLOSED;
	counter->open_timestamp = 0;

	customer_t customer;
	while((customer = queue_dequeue(counter->queue)) != NULL) {
		customer->current_queue = -1;
	}

	SUPERMARKET_LOG("Counter %d has been closed\n", counter->id);
}

void counter_destroy(counter_t counter) {
	PTHREAD_MUTEX_DESTROY_ERR(&counter->mtx);
	PTHREAD_COND_DESTROY_ERR(&counter->idle);

	queue_destroy(counter->queue);
	queue_destroy(counter->open_time);
	queue_destroy(counter->client_time);

	if(counter->open_timestamp != 0)
		SUPERMARKET_ERROR("Counter %d hasn't been saved\n", counter->id);

	free(counter);
}
