#ifndef _LOGGER_H
#define _LOGGER_H

#include "counter.h"
#include "customer.h"

void logger_init(char *filepath);
void logger_log_customer_data(customer_t customer);
void logger_log_counter_data(counter_t counter);
void logger_log_general_data(int signal);
void logger_cleanup();

#endif
