#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

#include "logger.h"

typedef struct {
	int n_customers;
	int n_products;
} GeneralData;

// Logger functions will be called by 'dying' customers, even if one
// lock limits the performance, in this context it's not that important
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int file = -1;
static enum { NONE, GENERAL, CLIENT, COUNTERS } current_mode = NONE;
static GeneralData *general_data = NULL;

void logger_init(char *filepath) {
	// Lock mutex
	pthread_mutex_lock(&mutex);
	
	if(file != -1) {
		// File has already been opened, no need to do it again
		return;
	}
	// Try to open the file
	if((file = open(filepath, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0666)) == -1) {
		perror("While opening the log file");
		return;
	}

	// Initialize general block data
	general_data = (GeneralData*) malloc(sizeof(GeneralData));
	general_data->n_customers = 0;
	general_data->n_products = 0;

	// Unlock mutex
	pthread_mutex_unlock(&mutex);
}

void logger_log_customer_data(int customer_id, int tot_time, int que_time, int n_products, int que_changes) {
	// Lock mutex
	pthread_mutex_lock(&mutex);
	// Makes sure everything has been initialized
	if (file == -1) {
		pthread_mutex_unlock(&mutex);
		return;
	}

	general_data->n_customers++;
	general_data->n_products += n_products;

	if (current_mode != CLIENT) {
		dprintf(file, "-- CLIENTI\n");
		current_mode = CLIENT;
	}
	dprintf(file, "%d:\n", customer_id);
	dprintf(file, "\tTEMPO NEL SUPERMERCATO: %d\n", tot_time);
	dprintf(file, "\tTEMPO IN CODA: %d\n", que_time);
	dprintf(file, "\tNUMERO PRODOTTI: %d\n", n_products);
	dprintf(file, "\tNUMERO CAMBI CODA: %d\n", que_changes);

	// Unlock
	pthread_mutex_unlock(&mutex);
}

void logger_log_general_data(int signal) {
	pthread_mutex_lock(&mutex);
	if (current_mode != GENERAL) {
		dprintf(file, "-- GENERALE\n");
		current_mode = GENERAL;
	}
	dprintf(file, "NUMERO CLIENTI: %d\n", general_data->n_customers);
	dprintf(file, "PRODOTTI VENDUTI: %d\n", general_data->n_products);
	dprintf(file, "SIGNAL: %s\n", signal == SIGHUP ? "SIGHUP" : "SIGQUIT");
	pthread_mutex_unlock(&mutex);
}

void logger_cleanup() {
	pthread_mutex_lock(&mutex);
	// Close file and set file descriptor to -1, blocking all other function calls
	close(file);
	free(general_data);
	file = -1;
	pthread_mutex_unlock(&mutex);
}
