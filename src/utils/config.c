#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "config.h"

int K = -1, C = -1, E = -1, T = -1, P = -1, I = -1, W = -1;

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
	size_t size = sizeof(char) * (max_line_length - 1);

	// Read line by line
	while(getline(&line, &size, file_fd) > 0) {
		if(strlen(line) < 3) continue;
		if(line[1] != ':') continue;
		int number = atoi(line + 2);
		switch(line[0]) {
			case 'K':
				K = number;
				break;
			case 'C':
				C = number;
				break;
			case 'E':
				E = number;
				break;
			case 'T':
				T = number;
				break;
			case 'P':
				P = number;
				break;
			case 'I':
				I = number;
				break;
			case 'W':
				W = number;
				break;
		}
	}
	
	free(line);
	fclose(file_fd);
	
	// Check if all necessary data is provided
	if(C == -1 || K == -1 || E == -1 || T == -1 
			|| P == -1 || I == -1 || W == -1) {
		fprintf(stderr, "Not all parameters were provided in config file\n");
		exit(EXIT_FAILURE);
	}
}
