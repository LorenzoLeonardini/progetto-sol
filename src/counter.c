#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <errno.h>

#include "counter.h"

counter_t counter_create(int id) {
	counter_t counter = (counter_t) malloc(sizeof(counter_struct_t));
	counter->id = id;
	counter->queue = queue_create();
	counter->status = OPEN;
	int err = pthread_mutex_init(&counter->mtx, NULL);
	if(err != 0) {
		errno = err;
		perror("Creating mutex");
		exit(EXIT_FAILURE);
	}
	return counter;
}

void counter_change_status(counter_t counter, status_t status) {
	pthread_mutex_lock(&counter->mtx);
	counter->status = status;
	pthread_mutex_unlock(&counter->mtx);
}

void counter_add_client(counter_t counter, client_t client) {
	pthread_mutex_lock(&counter->mtx);

	if(counter->status != OPEN) {
		fprintf(stderr, "Trying to add a client to a closed counter\n");
	} else {
		queue_add(counter->queue, client);
	}
	
	pthread_mutex_unlock(&counter->mtx);
}

void counter_delete(counter_t counter) {
	int err = pthread_mutex_destroy(&counter->mtx);
	if(err != 0) {
		errno = err;
		perror("Destroying mutex");
		exit(EXIT_FAILURE);
	}
	queue_delete(counter->queue);
	free(counter);
}
