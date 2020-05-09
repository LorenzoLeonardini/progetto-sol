#include <stdio.h>
#include <stdlib.h>

#include "utils/config.h"

#include "customer.h"

customer_t customer_create(int id) {
	customer_t customer = (customer_t) malloc(sizeof(customer_struct_t));
	customer->id = id;
	customer->shopping_time = 10 + (rand() % (T - 10));
	customer->products = rand() % P;
	customer->current_queue = -1;
	return customer;
}
