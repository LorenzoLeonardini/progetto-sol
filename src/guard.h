#ifndef _GUARD_H
#define _GUARD_H

void *guard_create(void *attr);
void guard_customer_exiting(unsigned int id);
void guard_close();

#endif
