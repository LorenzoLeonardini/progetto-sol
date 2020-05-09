#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

#include "utils/config.h"
#include "utils/consts.h"
#include "guard.h"
#include "logger.h"

#include "customer.h"

customer_t customer_create(int id) {
	customer_t customer = (customer_t) malloc(sizeof(customer_struct_t));
	customer->id = id;
	customer->shopping_time = 10 + (rand() % (T - 9));
	customer->products = rand() % (P + 1);
	customer->current_queue = -1;
	customer->being_served = FALSE;
	return customer;
}

void customer_delete(customer_t customer) {
	assert(customer->current_queue == -1);
	free(customer);
}

void *customer_thread_fnc(void *attr) {
	pthread_detach(pthread_self());
	customer_t customer = (customer_t) attr;

	// Do shopping
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = customer->shopping_time * 1000000L;
	nanosleep(&tim, NULL);

	logger_log_customer_data(customer->id, customer->shopping_time, 0, customer->products, 0);
	guard_customer_exiting();
	customer_delete(customer);
	return NULL;
}
