#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "llds/errors.h"
#include "utils/config.h"
#include "utils.h"

#include "guard.h"
#include "logger.h"
#include "supermarket.h"

#include "customer.h"

static void customer_ask_permission(customer_t customer);
static int customer_enqueue(customer_t customer);
static int customer_sleep(customer_t customer, struct timespec tim);
static void customer_consider_queue_change(customer_t customer);
static void customer_exit(customer_t customer);
static int random_shopping_time();
static int random_products_count();
static void register_handlers();

customer_t customer_create(unsigned int id) {
	customer_t customer = (customer_t) malloc(sizeof(customer_struct_t));
	customer->id = id;
	customer->shopping_time = random_shopping_time();
	customer->products = random_products_count();
	customer->patience_level = rand() % 10;

	customer->current_queue = -1;
	customer->visited_queues = 0;
	customer->queue_time = 0;
	customer->total_time = current_time_millis();
	customer->status = SHOPPING;

	PTHREAD_MUTEX_INIT_ERR(&customer->mtx, NULL);
	PTHREAD_COND_INIT_ERR(&customer->waiting_service, NULL);
	return customer;
}

void *customer_thread_fnc(void *attr) {
	register_handlers();
	pthread_detach(pthread_self());
	customer_t customer = (customer_t) attr;

	// Do shopping
	if(!customer_sleep(customer, millis_to_timespec(customer->shopping_time)))
		return NULL;

	PTHREAD_MUTEX_LOCK(&customer->mtx);
	customer->status = QUEUE;

	if(customer->products == 0) {
		customer->status = EXITING;
		// Need to ask the manager permission to exit
		customer_ask_permission(customer);
		customer_exit(customer);
		return NULL;
	}

	// The customer has to pay some products
	struct timespec tim = millis_to_timespec(S);

	customer->queue_time = current_time_millis();
	while(customer->status == QUEUE) {
		if(customer_enqueue(customer) == -1) {
			// No open counter
			customer->products = 0;
			customer->queue_time = current_time_millis()
				- customer->queue_time;
			customer_exit(customer);
			return NULL;
		}

		// Loop to periodically decide if to change queue
		while(customer->status == QUEUE && customer->current_queue != -1) {
			PTHREAD_MUTEX_UNLOCK(&customer->mtx);

			if(!customer_sleep(customer, tim))
				return NULL;

			PTHREAD_MUTEX_LOCK(&customer->mtx);
			// Could have been "removed" from queue or served
			if (customer->current_queue == -1 || customer->status != QUEUE)
				continue;
			customer_consider_queue_change(customer);
		}
	}
	while(customer->status != SERVED) {
		PTHREAD_COND_WAIT(&customer->waiting_service, &customer->mtx);
	}

	// Logging and deleting process
	customer_exit(customer);
	return NULL;
}

/**
 * If a customer is not buying anything, they need to ask the manager permission
 * to exit the supermarket.
 */
static void customer_ask_permission(customer_t customer) {
	// Permission should always be granted and there should be no error
	// However, I prefer trying a couple of times and then crashing
	// displaying why, instead of ignoring errors and having a buggy program
	int valid = 0, attempts = 3;
	do {
		int socket = connect_to_manager_server();
		int message[2] = { SO_CUSTOMER_REQUEST_EXIT, customer->id };
		write(socket, message, sizeof(int) * 2);
		int bytes;
		do {
			bytes = read(socket, message, sizeof(int) * 2);
		} while(bytes == -1 && errno == EINTR);
		close(socket);

		// Checking validity of message and handling error
		valid = message[0] == SO_CUSTOMER_GRANT_EXIT
			&& message[1] == customer->id;
		if(!valid) {
			SUPERMARKET_ERROR("Customer %u wasn't able to get permission from"
				   " manager. Retrying...\n", customer->id);
			sleep(1);
		}
	} while(!valid && attempts--);
	// If still not valid, then attempts is 0
	if(!valid) {
		SUPERMARKET_ERROR("Customer %u never got permission from manager.\n",
				customer->id);
		exit(EXIT_FAILURE);
	}
}

/**
 * Find a counter for the customer. In case all the counters are closed
 * (supermarket closed with signal SIGQUIT), returns -1, otherwise returns
 * counter number
 */
static int customer_enqueue(customer_t customer) {
	assert(customer->status == QUEUE && customer->current_queue == -1);
	rw_lock_start_read(counters_status);
	if(opened_counters == 0 || supermarket_opened == FALSE) {
		rw_lock_stop_read(counters_status);
		return -1;
	}
	int counter = rand() % opened_counters;
	counter_add_customer(counters[counter], customer);
	customer->current_queue = counter;
	customer->visited_queues++;
	rw_lock_stop_read(counters_status);
	return customer->current_queue;
}

/**
 * Try to nanosleep. In case it fails, if it's because of an interrupt then the
 * customer exit the supermarket immediately. If it just doesn't work the
 * program crashes.
 */
static int customer_sleep(customer_t customer, struct timespec tim) {
	struct timespec rem;
	int ret = nanosleep(&tim, &rem);
	if(ret == -1) {
		if(errno != EINTR) {
			perror("Customer shopping");
			SUPERMARKET_ERROR("Customer [%u] haven't waited %lu.%lu and "
					"wasn't stopped by signal\n", customer->id, rem.tv_sec,
					rem.tv_nsec);
			exit(EXIT_FAILURE);
		}
		PTHREAD_MUTEX_LOCK(&customer->mtx);
		customer->products = 0;
		customer_exit(customer);
		return 0;
	}
	return 1;
}

/**
 * Decide if customer is gonna change queue. In that case, exit from the current
 * one. The customer loop will take care of chosing another counter.
 */
static void customer_consider_queue_change(customer_t customer) {
	assert(customer->current_queue != -1);
	// Count number of customers in the same queue
	counter_t current_counter = counters[customer->current_queue];
	PTHREAD_MUTEX_LOCK(&current_counter->mtx);
	if(current_counter->queue->size > S2 * 2
		&& rand() % 10 > customer->patience_level) {
		// It could happen that the counter has started serving the customer,
		// but it hasn't updated the "being_served" flag yet. Or the counter has
		// told the customer to change queue because of a closure. In that case
		// it's not in the queue anymore and the `remove` returns NULL
		if(queue_remove(current_counter->queue, customer) != NULL)
			customer->current_queue = -1;
	}
	PTHREAD_MUTEX_UNLOCK(&current_counter->mtx);
}

/**
 * Log the customer data, inform the guard, unlock the mutex, free the memory
 */
static void customer_exit(customer_t customer) {
	customer->status = EXITING;
	logger_log_customer_data(customer);
	guard_customer_exiting(customer->id);
	PTHREAD_MUTEX_UNLOCK(&customer->mtx);
	customer_destroy(customer);
}

static int random_shopping_time() {
	return 10 + (rand() % (T - 9));
}

static int random_products_count() {
	return rand() % (P + 1);
}

void customer_destroy(customer_t customer) {
	assert(customer->current_queue == -1);
	PTHREAD_MUTEX_DESTROY_ERR(&customer->mtx);
	PTHREAD_COND_DESTROY_ERR(&customer->waiting_service);
	free(customer);
}

static void handler(int signum) {}

static void register_handlers() {
	struct sigaction s;
	sigset_t set;
	// Block signals until handler is installed
	SIG_FNC_ERR(sigfillset(&set));
	pthread_sigmask(SIG_SETMASK, &set, NULL);

	memset(&s, 0, sizeof(s));
	s.sa_handler = handler;
	sigaction(SIGUSR1, &s, NULL);
	// Unblock
	SIG_FNC_ERR(sigemptyset(&set));
	SIG_FNC_ERR(sigaddset(&set, SIGQUIT));
	SIG_FNC_ERR(sigaddset(&set, SIGHUP));
	pthread_sigmask(SIG_SETMASK, &set, NULL);
}
