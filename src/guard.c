#include <pthread.h>

#include "guard.h"
#include "utils/config.h"

static int client_n;
static int current_client_n;
static int should_close = 0;
static pthread_mutex_t guard_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t guard_cond = PTHREAD_COND_INITIALIZER;

static void create_client() {
	// TODO: create client
	client_n++;
	current_client_n++;
}

void *guard_create(void *attr) {
	pthread_mutex_lock(&guard_mtx);
	client_n = 0;
	current_client_n = 0;
	while(!should_close) {
		while(current_client_n < C) {
			create_client();
		}
		while(C - current_client_n < E && !should_close) {
			pthread_cond_wait(&guard_cond, &guard_mtx);
		}
	}
	pthread_mutex_unlock(&guard_mtx);
	return NULL; // Useless, here to remove compiler warning
}

void guard_client_exiting() {
	pthread_mutex_lock(&guard_mtx);
	current_client_n--;
	pthread_cond_signal(&guard_cond);
	pthread_mutex_unlock(&guard_mtx);
}

void guard_close() {
	pthread_mutex_lock(&guard_mtx);
	should_close = 1;
	pthread_cond_signal(&guard_cond);
	pthread_mutex_unlock(&guard_mtx);
}
