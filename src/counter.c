#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "llds/queue.h"
#include "llds/read_write_lock.h"
#include "utils.h"
#include "utils/config.h"
#include "customer.h"
#include "supermarket.h"

#include "counter.h"

static void counter_close(counter_t counter);
static msec_t counter_to_next_notification(counter_t counter);
static void counter_sleep(counter_t counter, msec_t millis);
static void counter_notify_manager(counter_t counter);

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
		PTHREAD_COND_SIGNAL(&counter->idle);
	}

	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
}

void *counter_thread_fnc(void *args) {
	block_quit_hup_handlers();
	counter_t counter = (counter_t) args;
	PTHREAD_MUTEX_LOCK(&counter->mtx);

	counter->open_timestamp = current_time_millis();
	counter->status = OPEN;
	counter->timer = counter->open_timestamp;
	rw_lock_stop_write(counters_status);

	struct timespec tim;
	while(counter->status != CLOSED) {
		// No customer in queue
		while(counter->status == OPEN && counter->queue->size == 0) {
			// Wait on idle condition, but wake up periodically to send
			// notification to manager
			tim = millis_to_timespec(counter_to_next_notification(counter)
									+ current_time_millis());

			if((errno = pthread_cond_timedwait(&counter->idle,
				&counter->mtx, &tim)) != 0) {
				if(errno == ETIMEDOUT) {
					if(current_time_millis() >= counter->timer + NOTIFY_TIME)
						counter_notify_manager(counter);
				} else {
					perror("Timedwait");
					exit(EXIT_FAILURE);
				}
			}
		}

		if(counter->status == CLOSING) {
			counter_close(counter);
		}

		if(counter->status == OPEN) {
			// Get customer from queue
			customer_t customer = queue_dequeue(counter->queue);
			// I'm not working on the counter ds, no need to keep the lock
			PTHREAD_MUTEX_UNLOCK(&counter->mtx);

			// Save timing data into counter ds
			PTHREAD_MUTEX_LOCK(&customer->mtx);
			msec_t start = current_time_millis();
			customer->being_served = 1;
			customer->queue_time = start - customer->queue_time;
			int prods = customer->products;
			PTHREAD_MUTEX_UNLOCK(&customer->mtx);

			// This is private data, no other thread access this
			counter->tot_customers++;
			counter->tot_products += prods;

			counter_sleep(counter, counter->time_for_customer
				+ prods * PRODUCT_TIME);

			PTHREAD_MUTEX_LOCK(&customer->mtx);
			customer->served = 1;
			customer->current_queue = -1;

			msec_t *t = (msec_t*) malloc(sizeof(msec_t));
			*t = current_time_millis() - start;
			queue_enqueue(counter->client_time, t);

			PTHREAD_COND_SIGNAL(&customer->waiting_service);
			PTHREAD_MUTEX_UNLOCK(&customer->mtx);
			PTHREAD_MUTEX_LOCK(&counter->mtx);
		}
	}

	PTHREAD_MUTEX_UNLOCK(&counter->mtx);
	return NULL;
}

/*
 * Returns milliseconds remaining to next notification
 */
static msec_t counter_to_next_notification(counter_t counter) {
	msec_t now = current_time_millis();
	if(now > counter->timer + NOTIFY_TIME) return 0;
	return counter->timer + NOTIFY_TIME - now;
}

/*
 * Sleep for a number of milliseconds. In the meanwhile, if it needs to send
 * a notification, that't taken care of
 */
static void counter_sleep(counter_t counter, msec_t millis) {
	msec_t to_notification = counter_to_next_notification(counter);
	msec_t waiting = millis;
	int has_to_split = 0;
	if(to_notification < millis) {
		waiting = to_notification;
		has_to_split = 1;
	}

	struct timespec tim = millis_to_timespec(waiting);
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

	if(has_to_split) {
		PTHREAD_MUTEX_LOCK(&counter->mtx);
		counter_notify_manager(counter);
		PTHREAD_MUTEX_UNLOCK(&counter->mtx);

		counter_sleep(counter, millis - waiting);
	}
}

static void counter_notify_manager(counter_t counter) {
	rw_lock_start_read(counters_status);

	int conn = connect_to_manager_server();
	int message[4] = { SO_COUNTER_QUEUE, opened_counters, counter->id,
				counter->queue->size };
	write(conn, message, sizeof(int) * 4);
	counter->timer = current_time_millis();
	write(conn, &counter->timer, sizeof(msec_t));
	close(conn);

	rw_lock_stop_read(counters_status);
}

static void counter_close(counter_t counter) {
	// Save open time details
	counter->opening_count++;
	msec_t *t = (msec_t*) malloc(sizeof(msec_t));
	*t = current_time_millis() - counter->open_timestamp;
	queue_enqueue(counter->open_time, t);

	// Change status
	counter->status = CLOSED;
	counter->open_timestamp = 0;

	// "Telling" every customer in queue to go to another counter
	customer_t customer;
	while((customer = queue_dequeue(counter->queue)) != NULL) {
		PTHREAD_MUTEX_LOCK(&customer->mtx);
		customer->current_queue = -1;
		PTHREAD_MUTEX_UNLOCK(&customer->mtx);
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
