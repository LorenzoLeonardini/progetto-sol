#ifndef _COUNTER_H
#define _COUNTER_H

#include <pthread.h>

#include "llds/queue.h"
#include "utils.h"
#include "customer.h"

typedef enum { OPEN, CLOSED, CLOSING } counter_status_t;
typedef struct {
	// Counter main data
	unsigned int id;
	queue_t queue;
	counter_status_t status;
	// Time to handle customer
	int time_for_customer;
	// Data
	int tot_customers;
	int tot_products;
	// For keeping track of how many times it's opened and for how long
	int opening_count;
	msec_t open_timestamp;
	queue_t open_time;
	queue_t client_time;
	// Multithread
	pthread_t thread;
	pthread_mutex_t mtx;
	pthread_cond_t idle;
	// Timer
	msec_t timer;
} counter_struct_t;
typedef counter_struct_t *counter_t;

counter_t counter_create(unsigned int id);
void counter_add_customer(counter_t counter, customer_t customer);
void *counter_thread_fnc(void *args);
void counter_destroy(counter_t counter);

#endif
