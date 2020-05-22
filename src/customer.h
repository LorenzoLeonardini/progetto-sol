#ifndef _CLIENT_H
#define _CLIENT_H

typedef struct {
	unsigned int id;
	int shopping_time;
	int products;
	int current_queue;
	short being_served;
	pthread_cond_t waiting_in_line;
} customer_struct_t;
typedef customer_struct_t *customer_t;

customer_t customer_create(unsigned int id);
void customer_destroy(customer_t customer);

void *customer_thread_fnc(void *attr);

#endif
