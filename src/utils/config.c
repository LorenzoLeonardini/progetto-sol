#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "config.h"

#define COMPARE(x) if(x == -1 && strcmp(line, #x) == 0) x = number;
#define COMPARE_STR(x) if(x[0] == '\0' && strcmp(line, #x) == 0) \
	strncpy(x, line + index, 99);

int K = -1, C = -1, E = -1, T = -1, P = -1, INITIAL_K = -1, PRODUCT_TIME = -1,
	NOTIFY_TIME = -1, S = -1, S1 = -1, S2 = -1;
char LOG_FILE[100] = { '\0' };

static void read_config_file(char *file);

void read_config(int argc, char **argv) {
	// Check program arguments
	if(argc != 1 && argc != 2) {
		fprintf(stderr, "WRONG USAGE:\n"
				"\t%s [config_file]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// argv[1] overrides default config file
	if(argc == 2) {
		read_config_file(argv[1]);
	} else {
		read_config_file("config.txt");
	}
}

static void read_config_file(char *file) {
	// Open file
	FILE *file_fd = fopen(file, "r");
	if(file_fd == NULL) {
		perror("Open config file");
		exit(EXIT_FAILURE);
	}

	// Prepare data to read line
	short max_line_length = 100;
	char *line = (char*) malloc(sizeof(char) * max_line_length);
	size_t size = sizeof(char) * (max_line_length);

	// Read line by line
	int len;
	while((len = getline(&line, &size, file_fd)) > 0) {
		// This should be called if line was too large for buffer and couldn't
		// fit completely.
		if(len >= 99) {
			free(line);
			fclose(file_fd);
			fprintf(stderr, "config file has too long line\n");
			exit(EXIT_FAILURE);
		}

		int index = -1;
		// Search ':' inside line and save its index
		for(int i = 0; i < 100 && index == -1; i++) {
			if(line[i] == ':')
				index = i;
			if(line[i] == '\0') break;
		}
		// There was no ':'
		if(index == -1) {
			fprintf(stderr, "WARNING: Invalid config line\n\t%s\n", line);
			continue;
		}
		// Replacing ':' with '\0', so that "line" contains var name and
		// "line + index" contains the value
		line[index++] = '\0';
		int number = atoi(line + index);
		// This removes the final \n and replace it with \0
		line[index + strlen(line + index) - 1] = '\0';

		COMPARE(K)
		else COMPARE(C)
		else COMPARE(E)
		else COMPARE(T)
		else COMPARE(P)
		else COMPARE(INITIAL_K)
		else COMPARE_STR(LOG_FILE)
		else COMPARE(PRODUCT_TIME)
		else COMPARE(NOTIFY_TIME)
		else COMPARE(S)
		else COMPARE(S1)
		else COMPARE(S2)
	}

	free(line);
	fclose(file_fd);

	// Check if all necessary data is provided
	if(K == -1 || C == -1 || E == -1 || T == -1 || P == -1 || INITIAL_K == -1
			|| LOG_FILE[0] == '\0' || PRODUCT_TIME == -1 || NOTIFY_TIME == -1
			|| S == -1 || S1 == - 1 || S2 == - 1) {
		fprintf(stderr, "Not all parameters were provided in config file\n");
		exit(EXIT_FAILURE);
	}
}
