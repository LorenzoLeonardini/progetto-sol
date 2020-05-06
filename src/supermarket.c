#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "supermarket.h"
#include "consts.h"

void supermarket_launch() {
	printf("[%d] SONO IL SUPERMERCATO\n", getpid());

	int socket_fd;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKET_PATH("man_sup"), UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	int max_attempts = 10;

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	while(connect(socket_fd, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
		if(errno == ENOENT) {
			if(!max_attempts--) {
				fprintf(stderr, "Connection timed out\n");
				exit(EXIT_FAILURE);
			}
			sleep(1);
		} else {
			exit(EXIT_FAILURE);
		}
	}
	close(socket_fd);
}
