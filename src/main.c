#include <sys/stat.h>
#include <unistd.h>
#include "consts.h"

#include "manager.h"
#include "supermarket.h"

int main(void) {
	// TODO: read config
	
	// Create sockets folder
	struct stat st = {0};
	if(stat("./sockets", &st) == -1) {
		mkdir("./sockets", 0755);
	}

	int supermarket_pid;
	if ((supermarket_pid = fork()) != 0) {
		manager_launch(supermarket_pid);
	} else {
		supermarket_launch();
	}

	return 0;
}
