#include <stdio.h>
#include <stdlib.h>

#include "../queue.h"

typedef struct {
	int value;
} number;

int main(void) {
	queue_t queue = queue_create();
	number *n1 = malloc(sizeof(number));
	number *n2 = malloc(sizeof(number));
	number *n3 = malloc(sizeof(number));
	number *n4 = malloc(sizeof(number));
	number *n5 = malloc(sizeof(number));
	n1->value = 1;
	n2->value = 2;
	n3->value = 3;
	n4->value = 4;
	n5->value = 5;
	queue_add(queue, n1);
	queue_add(queue, n2);
	queue_add(queue, n3);
	queue_add(queue, n4);
	queue_add(queue, n5);

	number *curr;
	curr = (number*) queue_head(queue);
	printf("HEAD %d\n", curr->value);
	curr = (number*) queue_tail(queue);
	printf("TAIL %d\n", curr->value);
	queue_remove(queue, n3);
	while((curr = queue_pop(queue)) != NULL) {
		printf("QUEUE (%d): %d\n", queue->size, curr->value);
	}

	free(n1);
	free(n2);
	free(n3);
	free(n4);
	free(n5);

	queue_delete(queue);
	return 0;
}
