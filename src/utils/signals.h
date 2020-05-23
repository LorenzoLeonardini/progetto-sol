#ifndef _SIGNALS_H
#define _SIGNALS_H

#define SIG_FNC_ERR(fnc) \
	if(fnc == -1) { \
		perror("Installing signal handlers"); \
		exit(EXIT_FAILURE); \
	}

#define PTHREAD_SIGMASK(mode, set, oldset) \
	if((errno = pthread_sigmask(mode, set, oldset)) != 0) { \
		perror("Masking signal"); \
	}

void register_quit_hup_handlers(int restart, void (*handler)(int signum));
void block_quit_hup_handlers();

#endif
