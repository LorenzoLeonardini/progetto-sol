#include <pthread.h>

#include "guard.h"
#include "utils/config.h"

static int customer_n;
static int current_customer_n;
static int should_close = 0;
static pthread_mutex_t guard_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t guard_cond = PTHREAD_COND_INITIALIZER;

static void create_customer() {
	// TODO: create customer
	customer_n++;
	current_customer_n++;
}

void *guard_create(void *attr) {
	pthread_mutex_lock(&guard_mtx);
	customer_n = 0;
	current_customer_n = 0;
	while(!should_close) {
		while(current_customer_n < C) {
			create_customer();
		}
		while(C - current_customer_n < E && !should_close) {
			pthread_cond_wait(&guard_cond, &guard_mtx);
		}
	}
	pthread_mutex_unlock(&guard_mtx);
	return NULL; // Useless, here to remove compiler warning
}

void guard_customer_exiting() {
	pthread_mutex_lock(&guard_mtx);
	current_customer_n--;
	pthread_cond_signal(&guard_cond);
	pthread_mutex_unlock(&guard_mtx);
}

void guard_close() {
	pthread_mutex_lock(&guard_mtx);
	should_close = 1;
	pthread_cond_signal(&guard_cond);
	pthread_mutex_unlock(&guard_mtx);
}
