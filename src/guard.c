#include <pthread.h>
#include <stdio.h>

#include <stdlib.h>
#include <errno.h>

#include "utils/config.h"
#include "utils/errors.h"

#include "guard.h"

static int customer_n;
static int current_customer_n;
static int should_close = 0;
static int should_gentle_close = 0;
static pthread_mutex_t guard_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t guard_cond = PTHREAD_COND_INITIALIZER;

static void create_customer() {
	// TODO: create customer
	customer_n++;
	current_customer_n++;
}

void *guard_create(void *attr) {
	PTHREAD_MUTEX_LOCK(&guard_mtx);
	customer_n = 0;
	current_customer_n = 0;
	while(!should_close) {
		while(current_customer_n < C) {
			create_customer();
		}
		while(C - current_customer_n < E && !should_close) {
			PTHREAD_COND_WAIT(&guard_cond, &guard_mtx);
		}
	}
	if(should_gentle_close)
		while(current_customer_n > 0)
			PTHREAD_COND_WAIT(&guard_cond, &guard_mtx);
	PTHREAD_MUTEX_UNLOCK(&guard_mtx);
	pthread_exit(NULL);
}

void guard_customer_exiting() {
	PTHREAD_MUTEX_LOCK(&guard_mtx);
	current_customer_n--;
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
