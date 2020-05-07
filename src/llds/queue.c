#include <stdlib.h>

#include "queue.h"

queue_t queue_create() {
	queue_t queue = (queue_t) malloc(sizeof(queue_struct_t));
	queue->head = NULL;
	queue->tail = NULL;
	queue->size = 0;
	return queue;
}

void *queue_head(queue_t queue) {
	if(queue->head != NULL) {
		return queue->head->element;
	}
	return NULL;
}

void *queue_tail(queue_t queue) {
	if(queue->tail != NULL) {
		return queue->tail->element;
	}
	return NULL;
}

void queue_add(queue_t queue, void *element) {
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

void *queue_pop(queue_t queue) {
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

void queue_remove(queue_t queue, void *element) {
	if(queue->head == NULL) return;
	if(queue->head->element == element) {
		// If item to be removed is head we can use pop
		queue_pop(queue);
		return;
	}
	queue_element_t *current = queue->head;
	while(current->next != NULL) {
		if(current->next->element == element) {
			queue_element_t *next = current->next->next;
			free(current->next);
			current->next = next;
			if (current->next == NULL) {
				queue->tail = current;
			}
			queue->size--;
			return;
		}
		current = current->next;
	}
}

void queue_remove_all(queue_t queue) {
	while(queue->head != NULL) {
		queue_pop(queue);
	}
}

void queue_delete(queue_t queue) {
	if(queue->head != NULL) {
		queue_remove_all(queue);
	}
	free(queue);
}
