#ifndef _SUPERMARKET_H
#define _SUPERMARKET_H

#include "llds/read_write_lock.h"
#include "counter.h"

extern rw_lock_t counters_status;
extern counter_t *counters;
extern int opened_counters, supermarket_opened;

void supermarket_launch();

#endif
