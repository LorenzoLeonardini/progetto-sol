#ifndef _QUEUE_H
#define _QUEUE_H

typedef struct que_el {
	void *element;
	struct que_el *next;
} queue_element_t;

typedef struct {
	queue_element_t *head;
	queue_element_t *tail;
	int size;
} queue_struct_t;

typedef queue_struct_t *queue_t;

queue_t queue_create();
void *queue_head(queue_t queue);
void *queue_tail(queue_t queue);
void queue_add(queue_t queue, void *element);
void *queue_pop(queue_t queue);
void queue_remove(queue_t queue, void *element);
void queue_remove_all(queue_t queue);
void queue_delete(queue_t queue);

#endif
