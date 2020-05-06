#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "manager.h"
#include "consts.h"

void manager_launch(int supermarket_pid) {
	printf("[%d] SONO IL DIRETTORE (super %d)\n", getpid(), supermarket_pid);

	int socket_fd;
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKET_PATH("man_sup"), UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	unlink(SOCKET_PATH("man_sup"));
	bind(socket_fd, (struct sockaddr*) &sa, sizeof(sa));
	listen(socket_fd, SOMAXCONN);
	int connection_fd;
	while((connection_fd = accept(socket_fd, NULL, 0)) == -1) {
		perror("Waiting the supermarket connection");
	}
	printf("[Manager] new connection :D %d\n", connection_fd);
	close(connection_fd);
	close(socket_fd);
}
