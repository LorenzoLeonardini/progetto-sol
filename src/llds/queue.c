#include <stdlib.h>

#include "queue.h"

static int compare_pointer(const void *a, const void *b);

queue_t queue_create() {
	queue_t queue = (queue_t) malloc(sizeof(queue_struct_t));
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	return queue;
}

void *queue_head(const queue_t queue) {
	if(queue->head != NULL) {
		return queue->head->element;
	}
	return NULL;
}

void *queue_tail(const queue_t queue) {
	if(queue->tail != NULL) {
		return queue->tail->element;
	}
	return NULL;
}

void queue_enqueue(queue_t queue, void *element) {
	queue_element_t *qe = (queue_element_t*) malloc(sizeof(queue_element_t));
	qe->element = element;
	qe->next = NULL;
	if(queue->tail == NULL) {
		// New element is both head and tail
		queue->tail = qe;
		queue->head = qe;
	} else {
		// New element is new tail
		queue->tail->next = qe;
		queue->tail = qe;
	}
	queue->size++;
}

void *queue_dequeue(queue_t queue) {
	if(queue->head != NULL) {
		// Empty memory, make next element as new head and return value
		queue_element_t *next = queue->head->next;
		void *element = queue->head->element;
		free(queue->head);
		queue->head = next;
		queue->size--;
		if(queue->head == NULL) {
			queue->tail = NULL;
		}
		return element;
	}
	return NULL;
}

void *queue_remove(queue_t queue, const void *element) {
	return queue_remove_cmp(queue, element, compare_pointer);
}

void *queue_remove_cmp(queue_t queue, const void *element,
	int (*cmp)(const void*, const void*)) {

	int index = 0;
	if(queue->head == NULL) return NULL;
	if(cmp(queue->head->element, element) == 0) {
		// If item to be removed is head we can use pop
		return queue_dequeue(queue);
	}
	queue_element_t *current = queue->head;
	while(current->next != NULL) {
		index++;
		if(cmp(current->next->element, element) == 0) {
			queue_element_t *next = current->next->next;
			void *e = current->next->element;
			free(current->next);
			current->next = next;
			if (current->next == NULL) {
				queue->tail = current;
			}
			queue->size--;
			return e;
		}
		current = current->next;
	}
	return NULL;
}

void queue_remove_all(queue_t queue) {
	while(queue->head != NULL) {
		queue_dequeue(queue);
	}
}

void queue_destroy(queue_t queue) {
	if(queue->head != NULL) {
		queue_remove_all(queue);
	}
	free(queue);
}

static int compare_pointer(const void *a, const void *b) {
	if(a < b) return -1;
	if(a > b) return 1;
	return 0;
}
