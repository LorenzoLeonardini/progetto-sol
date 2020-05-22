#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "llds/queue.h"
#include "counter.h"
#include "customer.h"

#include "logger.h"

// Logger functions will be called by 'dying' customers, even if one
// lock limits the performance, in this context it's not that important
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int file = -1;
static enum { NONE, GENERAL, CLIENT, COUNTERS } current_mode = NONE;
static unsigned int n_customers;
static unsigned int n_products;

void logger_init(char *filepath) {
	PTHREAD_MUTEX_LOCK(&mutex);

	// File has already been opened, no need to do it again
	CHECK_FILE_NO_INIT(file, &mutex);
	// Try to open the file
	if((file = open(filepath, O_RDWR | O_CREAT | O_TRUNC
		| O_APPEND, 0666)) == -1) {
		SUPERMARKET_ERROR("There was an error while opening the log file\n");
		SUPERMARKET_ERROR("The program will continue, but nothing will be saved"
			" to file\n");
		perror("While opening the log file");
		PTHREAD_MUTEX_UNLOCK(&mutex);
		return;
	}

	// Initialize general block data
	n_customers = 0;
	n_products = 0;

	PTHREAD_MUTEX_UNLOCK(&mutex);
}

void logger_log_customer_data(const customer_t customer) {
	PTHREAD_MUTEX_LOCK(&mutex);
	// Makes sure everything has been initialized
	CHECK_FILE_INIT(file, &mutex);

	n_customers++;
	n_products += customer->products;

	if (current_mode != CLIENT) {
		dprintf(file, "-- CLIENTI\n");
		current_mode = CLIENT;
	}

	dprintf(file, "%u:\n", customer->id);
	dprintf(file, "\tTEMPO NEL SUPERMERCATO: %d\n", customer->shopping_time
			+ customer->products * 30);
	dprintf(file, "\tTEMPO IN CODA: %llu\n", customer->queue_time);
	dprintf(file, "\tNUMERO PRODOTTI: %d\n", customer->products);
	dprintf(file, "\tNUMERO CAMBI CODA: %d\n", customer->visited_queues);

	PTHREAD_MUTEX_UNLOCK(&mutex);
}

void logger_log_counter_data(const counter_t counter) {
	PTHREAD_MUTEX_LOCK(&mutex);
	// Makes sure everything has been initialized
	CHECK_FILE_INIT(file, &mutex);

	if (current_mode != COUNTERS) {
		dprintf(file, "-- CASSE\n");
		current_mode = COUNTERS;
	}

	msec_t *t;
	dprintf(file, "%d:\n", counter->id);
	dprintf(file, "\tNUMERO CLIENTI: %d\n", counter->tot_customers);

	dprintf(file, "\tTEMPO CLIENTI:\n");
	while((t = (msec_t*) queue_dequeue(counter->client_time)) != NULL) {
		dprintf(file, "\t\t%llu\n", *t);
		free(t);
	}

	dprintf(file, "\tNUMERO PRODOTTI: %d\n", counter->tot_products);
	dprintf(file, "\tNUMERO CHIUSURE: %d\n", counter->opening_count);

	dprintf(file, "\tAPERTURE:\n");
	while((t = (msec_t*) queue_dequeue(counter->open_time)) != NULL) {
		dprintf(file, "\t\t%llu\n", *t);
		free(t);
	}

	PTHREAD_MUTEX_UNLOCK(&mutex);
}

void logger_log_general_data(const int signal) {
	PTHREAD_MUTEX_LOCK(&mutex);
	// Makes sure everything has been initialized
	CHECK_FILE_INIT(file, &mutex);

	if (current_mode != GENERAL) {
		dprintf(file, "-- GENERALE\n");
		current_mode = GENERAL;
	}

	dprintf(file, "NUMERO CLIENTI: %d\n", n_customers);
	dprintf(file, "PRODOTTI VENDUTI: %d\n", n_products);
	dprintf(file, "SIGNAL: %s\n", signal == SIGHUP ? "SIGHUP" : "SIGQUIT");

	PTHREAD_MUTEX_UNLOCK(&mutex);
}

void logger_cleanup() {
	PTHREAD_MUTEX_LOCK(&mutex);
	// Close file and set file descriptor to -1, blocking all other function
	// calls
	if(file != -1) {
		close(file);
		file = -1;
	}
	PTHREAD_MUTEX_UNLOCK(&mutex);
}
