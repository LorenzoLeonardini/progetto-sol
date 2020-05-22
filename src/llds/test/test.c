#include <stdio.h>
#include <stdlib.h>

#include "../queue.h"
#include "../hashmap.h"

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
	queue_enqueue(queue, n1);
	queue_enqueue(queue, n2);
	queue_enqueue(queue, n3);
	queue_enqueue(queue, n4);
	queue_enqueue(queue, n5);

	number *curr;
	curr = (number*) queue_head(queue);
	printf("HEAD %d\n", curr->value);
	curr = (number*) queue_tail(queue);
	printf("TAIL %d\n", curr->value);
	queue_remove(queue, n3);
	while((curr = queue_dequeue(queue)) != NULL) {
		printf("QUEUE (%d): %d\n", queue->size, curr->value);
	}

	queue_destroy(queue);

	hashmap_t hm = hashmap_create(10);
	hashmap_add(hm, 3, n1);
	void *r = hashmap_remove(hm, 3);
	printf("eq %d\n", r == n1 ? 1 : 0);
	hashmap_destroy(hm);

	free(n1);
	free(n2);
	free(n3);
	free(n4);
	free(n5);

	return 0;
}
