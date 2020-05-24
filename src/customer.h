#ifndef _CLIENT_H
#define _CLIENT_H

typedef struct {
	unsigned int id;
	int shopping_time;
	int products;
	int patience_level;
	int current_queue;
	int visited_queues;
	msec_t queue_time;
	msec_t total_time;
	short being_served;
	short served;
	pthread_mutex_t mtx;
	pthread_cond_t waiting_service;
} customer_struct_t;
typedef customer_struct_t *customer_t;

customer_t customer_create(unsigned int id);
void *customer_thread_fnc(void *attr);
void customer_destroy(customer_t customer);

#endif
