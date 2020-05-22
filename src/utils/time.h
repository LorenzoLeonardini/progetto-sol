#ifndef _TIME_UTILS_H
#define _TIME_UTILS_H

typedef unsigned long long msec_t;

msec_t current_time_millis();
struct timespec millis_to_timespec(int millis);

#endif
