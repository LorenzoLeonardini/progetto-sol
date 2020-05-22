#ifndef _CONFIG_H
#define _CONFIG_H

extern int K, C, E, T, P, INITIAL_K, PRODUCT_TIME, NOTIFY_TIME, S, S1, S2;
extern char LOG_FILE[100];

void read_config(int argc, char **argv);

#endif
