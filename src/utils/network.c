#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "consts.h"

#include "network.h"

int connect_to_manager_server() {
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKET_PATH("man_sup"), UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	int max_attempts = 10;
	int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd == -1) {
		perror("[Supermarket] Connecting to manager socket");
		exit(EXIT_FAILURE);
	}
	while(connect(socket_fd, (struct sockaddr*) &sa, sizeof(sa)) == -1) {
		if(errno == ENOENT) {
			if(!max_attempts--) {
				fprintf(stderr, "Connection timed out\n");
				exit(EXIT_FAILURE);
			}
			sleep(1);
		} else {
			fprintf(stderr, "Can't connect to manager socket\n");
			exit(EXIT_FAILURE);
		}
	}
	return socket_fd;
}
