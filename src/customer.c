#include <stdio.h>
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "utils/config.h"
#include "utils.h"

#include "guard.h"
#include "logger.h"

#include "customer.h"

static int random_shopping_time();
static int random_products_count();
static void customer_ask_permission(customer_t customer);
static void customer_exit(customer_t customer);
static void register_handlers();

customer_t customer_create(unsigned int id) {
	customer_t customer = (customer_t) malloc(sizeof(customer_struct_t));
	customer->id = id;
	customer->shopping_time = random_shopping_time();
	customer->products = random_products_count();
	customer->current_queue = -1;
	customer->being_served = FALSE;
	return customer;
}

void customer_destroy(customer_t customer) {
	assert(customer->current_queue == -1);
	free(customer);
}

void *customer_thread_fnc(void *attr) {
	pthread_detach(pthread_self());
	register_handlers();
	customer_t customer = (customer_t) attr;

	// Do shopping
	struct timespec tim = millis_to_timespec(customer->shopping_time + customer->products * 300);
	int ret = nanosleep(&tim, &tim);
	if(ret == -1) {
		if(errno != EINTR) {
			perror("Customer shopping");
			SUPERMARKET_ERROR("Customer [%u] haven't waited %lu.%lu and wasn't stopped by signal\n", customer->id, tim.tv_sec, tim.tv_nsec);
			exit(EXIT_FAILURE);
		}
		customer->products = 0;
		customer_exit(customer);
		return NULL;
	}

	if(customer->products == 0) {
		// Need to ask the manager permission to exit
		customer_ask_permission(customer);
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
		valid = message[0] == SO_CUSTOMER_GRANT_EXIT && message[1] == customer->id;
		if(!valid) { 
			SUPERMARKET_ERROR("Customer %u wasn't able to get permission from manager. Retrying...\n", customer->id);
			sleep(1);
		}
	} while(!valid && attempts--);
	// If still not valid, then attempts is 0
	if(!valid) {
		SUPERMARKET_ERROR("Customer %u never got permission from manager.\n", customer->id);
		exit(EXIT_FAILURE);
	}
}

static void customer_exit(customer_t customer) {
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
	memset(&s, 0, sizeof(s));
	s.sa_handler = gestore;
	sigaction(SIGUSR1, &s, NULL);
}
