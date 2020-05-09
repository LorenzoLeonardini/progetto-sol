#ifndef _CLIENT_H
#define _CLIENT_H

typedef struct {
	int id;
	int shopping_time;
	int products;
	int current_queue;
	int being_served;
} customer_struct_t;
typedef customer_struct_t *customer_t;

customer_t customer_create(int id);
void customer_delete(customer_t customer);

void *customer_thread_fnc(void *attr);

#endif
