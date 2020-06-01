#ifndef _CLIENT_H
#define _CLIENT_H

typedef enum { SHOPPING, QUEUE, BEING_SERVED, SERVED, EXITING } customer_status_t;

typedef struct {
	unsigned int id;
	int shopping_time;
	int products;
	int patience_level;
	int current_queue;
	int visited_queues;
	msec_t queue_time;
	msec_t total_time;
	customer_status_t status;
	pthread_mutex_t mtx;
	pthread_cond_t waiting_service;
} customer_struct_t;
typedef customer_struct_t *customer_t;

customer_t customer_create(unsigned int id);
void *customer_thread_fnc(void *attr);
void customer_destroy(customer_t customer);

#endif
