#ifndef _LOGGER_H
#define _LOGGER_H

#include "counter.h"
#include "customer.h"

#define CHECK_FILE_INIT(file, mutex) \
	if (file == -1) { \
		PTHREAD_MUTEX_UNLOCK(mutex); \
		return; \
	}

#define CHECK_FILE_NO_INIT(file, mutex) \
	if (file != -1) { \
		PTHREAD_MUTEX_UNLOCK(mutex); \
		return; \
	}

void logger_init(char *filepath);
void logger_log_customer_data(const customer_t customer);
void logger_log_counter_data(const counter_t counter);
void logger_log_general_data(const int signal);
void logger_cleanup();

#endif
