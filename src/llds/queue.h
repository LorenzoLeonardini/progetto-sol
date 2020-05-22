#ifndef _QUEUE_H
#define _QUEUE_H

typedef struct que_el {
	void *element;
	struct que_el *next;
} queue_element_t;

typedef struct {
	queue_element_t *head;
	queue_element_t *tail;
	unsigned int size;
} queue_struct_t;

typedef queue_struct_t *queue_t;

queue_t queue_create();
void *queue_head(const queue_t queue);
void *queue_tail(const queue_t queue);
void queue_enqueue(queue_t queue, void *element);
void *queue_dequeue(queue_t queue);
void *queue_remove(queue_t queue, const void *element);
void *queue_remove_cmp(queue_t queue, const void *element,
	int (*cmp)(const void*, const void*));
void queue_remove_all(queue_t queue);
void queue_destroy(queue_t queue);

#endif
