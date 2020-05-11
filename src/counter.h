#ifndef _COUNTER_H
#define _COUNTER_H

#include <pthread.h>

#include "llds/queue.h"
#include "customer.h"

typedef enum { OPEN, CLOSED, CLOSING } status_t;
typedef struct {
	int id;
	queue_t queue;
	pthread_mutex_t mtx;
	status_t status;
	int tot_customers;
	int tot_products;
	int open_count;
	unsigned long long open_timestamp;
	queue_t open_time;
} counter_struct_t;
typedef counter_struct_t *counter_t;

counter_t counter_create(int id);
void counter_change_status(counter_t counter, status_t status);
void counter_add_client(counter_t counter, customer_t customer);
int counter_queue_length(counter_t counter);
void counter_open(counter_t counter);
void counter_close(counter_t counter);
void counter_delete(counter_t counter);

#endif
