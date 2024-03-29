#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>

#include "utils/config.h"
#include "manager.h"
#include "supermarket.h"

int main(int argc, char **argv) {
	read_config(argc, argv);

	// Create sockets folder
	struct stat st = {0};
	if(stat("./sockets", &st) == -1) {
		mkdir("./sockets", 0755);
	}

	int supermarket_pid;
	if ((supermarket_pid = fork()) == -1) {
		perror("While trying to fork the process");
		exit(EXIT_FAILURE);
	} else if(supermarket_pid != 0) {
		manager_launch(supermarket_pid);
	} else {
		supermarket_launch();
	}

	return 0;
}
