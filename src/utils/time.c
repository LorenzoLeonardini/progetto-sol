#include <stdlib.h>
#include <sys/time.h>

#include "time.h"

msec_t current_time_millis() {
	struct timeval t;
	gettimeofday(&t, NULL);
	unsigned long long seconds = t.tv_sec;
	unsigned long long useconds = t.tv_usec;
	return (seconds * 1000000L + useconds) / 1000L;
}

struct timespec millis_to_timespec(msec_t millis) {
	struct timespec tim;
	millis *= 1000000L;
	tim.tv_sec  = millis / 1000000000L;
	tim.tv_nsec = millis % 1000000000L;
	return tim;
}
