#ifndef _HASHMAP_H
#define _HASHMAP_H

#include "queue.h"

typedef struct {
	int key;
	void *element;
} hashmap_element_t;

typedef struct {
	queue_t *arr;
	int element_count;
	int size;
} hashmap_struct_t;

typedef hashmap_struct_t *hashmap_t;

hashmap_t hashmap_create(int size);
void hashmap_add(hashmap_t hashmap, int key, void *element);
void *hashmap_remove(hashmap_t hashmap, int key);
void *hashmap_find(hashmap_t hashmap, int key);
queue_t hashmap_to_queue(hashmap_t);
void hashmap_destroy(hashmap_t hashmap);

#endif
