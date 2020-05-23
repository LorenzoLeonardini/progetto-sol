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

static int random_shopping_time();
static int random_products_count();
static void customer_ask_permission(customer_t customer);
static int customer_enqueue(customer_t customer);
static void customer_exit(customer_t customer);
static void register_handlers();

customer_t customer_create(unsigned int id) {
	customer_t customer = (customer_t) malloc(sizeof(customer_struct_t));
	customer->id = id;
	customer->shopping_time = random_shopping_time();
	customer->products = random_products_count();
	customer->current_queue = -1;
	customer->visited_queues = 0;
	customer->queue_time = 0;
	customer->being_served = FALSE;
	customer->served = FALSE;
	PTHREAD_MUTEX_INIT_ERR(&customer->mtx, NULL);
	PTHREAD_COND_INIT_ERR(&customer->waiting_in_line, NULL);
	return customer;
}

void customer_destroy(customer_t customer) {
	assert(customer->current_queue == -1);
	PTHREAD_MUTEX_DESTROY_ERR(&customer->mtx);
	PTHREAD_COND_DESTROY_ERR(&customer->waiting_in_line);
	free(customer);
}

void *customer_thread_fnc(void *attr) {
	block_quit_hup_handlers();
	pthread_detach(pthread_self());
	register_handlers();
	customer_t customer = (customer_t) attr;

	PTHREAD_MUTEX_LOCK(&customer->mtx);
	// Do shopping
	struct timespec tim = millis_to_timespec(customer->shopping_time);
	int ret = nanosleep(&tim, &tim);
	if(ret == -1) {
		if(errno != EINTR) {
			perror("Customer shopping");
			SUPERMARKET_ERROR("Customer [%u] haven't waited %lu.%lu and "
					"wasn't stopped by signal\n", customer->id, tim.tv_sec,
					tim.tv_nsec);
			exit(EXIT_FAILURE);
		}
		customer->products = 0;
		customer_exit(customer);
		return NULL;
	}

	if(customer->products == 0) {
		// Need to ask the manager permission to exit
		customer_ask_permission(customer);
	} else {
		customer->queue_time = current_time_millis();
		while(customer->current_queue == -1 && !customer->being_served && !customer->served) {
			if(customer_enqueue(customer) == -1) {
				customer->products = 0;
				customer->queue_time = current_time_millis()
					- customer->queue_time;
				customer_exit(customer);
				return NULL;
			}
			while(customer->current_queue != -1 && !customer->being_served && !customer->served) {
				PTHREAD_MUTEX_UNLOCK(&customer->mtx);
				sleep(1);
				PTHREAD_MUTEX_LOCK(&customer->mtx);
				if (customer->current_queue == -1) continue;
				counter_t current_counter = counters[customer->current_queue];
				PTHREAD_MUTEX_LOCK(&current_counter->mtx);
				if(current_counter->queue->size > S2) {
					customer->current_queue = -1;
					queue_remove(current_counter->queue, customer);
				}
				PTHREAD_MUTEX_UNLOCK(&current_counter->mtx);
			}
		}
		while(!customer->served) {
			PTHREAD_COND_WAIT(&customer->waiting_in_line, &customer->mtx);
		}
	}

	// Logging and deleting process
	customer_exit(customer);
	return NULL;
}

static void customer_ask_permission(customer_t customer) {
	// Permission should always be granted and there should be no error
	// However, I prefer trying a couple of times and then crashing
	// displaying why, instead of having a buggy program
	int valid = 0, attempts = 3;
	do {
		int socket = connect_to_manager_server();
		int message[2] = { SO_CUSTOMER_REQUEST_EXIT, customer->id };
		write(socket, message, sizeof(int) * 2);
		read(socket, message, sizeof(int) * 2);
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

static int customer_enqueue(customer_t customer) {
	rw_lock_start_read(counters_status);
	if(opened_counters == 0) {
		rw_lock_stop_read(counters_status);
		return -1;
	}
	int counter = rand() % opened_counters;
	PTHREAD_MUTEX_LOCK(&counters[counter]->mtx);
	queue_enqueue(counters[counter]->queue, customer);
	PTHREAD_COND_SIGNAL(&counters[counter]->idle);
	PTHREAD_MUTEX_UNLOCK(&counters[counter]->mtx);
	customer->current_queue = counter;
	customer->visited_queues++;
	rw_lock_stop_read(counters_status);
	return customer->current_queue;
}

static void customer_exit(customer_t customer) {
	PTHREAD_MUTEX_UNLOCK(&customer->mtx);
	logger_log_customer_data(customer);
	guard_customer_exiting(customer->id);
	customer_destroy(customer);
}

static int random_shopping_time() {
	return 10 + (rand() % (T - 9));
}

static int random_products_count() {
	return rand() % (P + 1);
}

static void gestore(int signum) {}

static void register_handlers() {
	struct sigaction s;
	sigset_t set;
	// Block signals until handler is installed
	SIG_FNC_ERR(sigfillset(&set));
	pthread_sigmask(SIG_SETMASK, &set, NULL);
	
	memset(&s, 0, sizeof(s));
	s.sa_handler = gestore;
	sigaction(SIGUSR1, &s, NULL);
	// Unblock
	SIG_FNC_ERR(sigemptyset(&set));
	pthread_sigmask(SIG_SETMASK, &set, NULL);
}
