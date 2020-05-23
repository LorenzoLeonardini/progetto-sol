#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <signal.h>
#include <string.h>

#include "signals.h"

void register_quit_hup_handlers(int restart, void (*handler)(int signum)) {
	struct sigaction s;
	sigset_t set1, set2;
	// Block signals until handler is installed
	SIG_FNC_ERR(sigfillset(&set1));
	pthread_sigmask(SIG_SETMASK, &set1, NULL);
	// Block SIGQUIT and SIGHUP in sigaction
	SIG_FNC_ERR(sigemptyset(&set2));
	SIG_FNC_ERR(sigaddset(&set2, SIGQUIT));
	SIG_FNC_ERR(sigaddset(&set2, SIGHUP));
	// Handle
	memset(&s, 0, sizeof(s));
	s.sa_handler = handler;
	s.sa_mask = set2;
	if(restart) {
		s.sa_flags = SA_RESTART;
	}
	SIG_FNC_ERR(sigaction(SIGQUIT, &s, NULL));
	SIG_FNC_ERR(sigaction(SIGHUP, &s, NULL));
	// Unblock signals
	SIG_FNC_ERR(sigemptyset(&set1));
	pthread_sigmask(SIG_SETMASK, &set1, NULL);
}

void block_quit_hup_handlers() {
	sigset_t set;
	SIG_FNC_ERR(sigfillset(&set));
	SIG_FNC_ERR(sigemptyset(&set));
	SIG_FNC_ERR(sigaddset(&set, SIGQUIT));
	SIG_FNC_ERR(sigaddset(&set, SIGHUP));
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}
