#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "logger.h"

int main(void) {
	srand(time(NULL));
	logger_init("asd.txt");
	for(int i = 0; i < 20; i++) {
		logger_log_client_data(i, rand() % 2000, rand() % 1000, rand() % 20, rand() % 4);
	}
	logger_log_general_data();
	logger_cleanup();
	return 0;
}
