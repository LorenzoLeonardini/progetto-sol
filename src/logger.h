#ifndef _LOGGER_H
#define _LOGGER_H

#include "counter.h"

void logger_init(char *filepath);
void logger_log_customer_data(int customer_id, int tot_time, int que_time, int n_products, int que_changes);
void logger_log_counter_data(counter_t counter);
void logger_log_general_data(int signal);
void logger_cleanup();

#endif
