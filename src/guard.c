#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "llds/hashmap.h"
#include "utils/config.h"
#include "utils.h"
#include "customer.h"

#include "guard.h"

static unsigned int customer_n;
static unsigned int current_customer_n;
static hashmap_t customer_threads;
static short should_close = 0;
static short should_gentle_close = 0;
static pthread_mutex_t guard_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t guard_cond = PTHREAD_COND_INITIALIZER;

/**
 * Create a customer, let them into the supermarket, add their thread to a
 * hashmap, increment counters and so on...
 */
static void create_customer() {
	// Customer creation
	customer_t customer = customer_create(customer_n);
	pthread_t *customer_thread = (pthread_t*) malloc(sizeof(pthread_t));
	hashmap_add(customer_threads, customer_n, customer_thread);
	PTHREAD_CREATE(customer_thread, NULL, &customer_thread_fnc, customer);
	// Incremention
	customer_n++;
	current_customer_n++;
	if(customer_n % 10 == 0) {
		SUPERMARKET_LOG("We've reached customer n.%d\n", customer_n);
	}
}

void *guard_create(void *attr) {
	block_quit_hup_handlers();
	srand(time(NULL));
	PTHREAD_MUTEX_LOCK(&guard_mtx);
	customer_n = 0;
	current_customer_n = 0;
	customer_threads = hashmap_create(C);
	// Guard loop
	while(!should_close) {
		while(current_customer_n < C) {
			create_customer();
		}
		while(C - current_customer_n < E && !should_close) {
			PTHREAD_COND_WAIT(&guard_cond, &guard_mtx);
		}
	}

	// Send every customer a signal to interrupt every sleep and exit the
	// supermarket
	if(!should_gentle_close) {
		SUPERMARKET_LOG("Forcing every customer to exit\n");
		queue_t threads = hashmap_to_queue(customer_threads);
		pthread_t *thread;
		while((thread = queue_dequeue(threads)) != NULL) {
			pthread_kill(*thread, SIGUSR1);
		}
		queue_destroy(threads);
	}

	// Wait for everyone to exit
	while(current_customer_n > 0) {
		if(current_customer_n % 10 == 0) {
			SUPERMARKET_LOG("%d customers are exiting the supermarket\n", current_customer_n);
		}
		PTHREAD_COND_WAIT(&guard_cond, &guard_mtx);
	}

	hashmap_destroy(customer_threads);
	PTHREAD_MUTEX_UNLOCK(&guard_mtx);
	return NULL;
}

void guard_customer_exiting(unsigned int id) {
	PTHREAD_MUTEX_LOCK(&guard_mtx);
	current_customer_n--;
	pthread_t *thread = hashmap_remove(customer_threads, id);
	free(thread);
	PTHREAD_COND_SIGNAL(&guard_cond);
	PTHREAD_MUTEX_UNLOCK(&guard_mtx);
}

void guard_close(int gentle_close) {
	PTHREAD_MUTEX_LOCK(&guard_mtx);
	should_close = 1;
	should_gentle_close = gentle_close;
	PTHREAD_COND_SIGNAL(&guard_cond);
	PTHREAD_MUTEX_UNLOCK(&guard_mtx);
}
