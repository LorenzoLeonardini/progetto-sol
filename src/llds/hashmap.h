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

hashmap_t hashmap_create(const int size);
void hashmap_add(hashmap_t hashmap, const int key, void *element);
void *hashmap_remove(hashmap_t hashmap, const int key);
void *hashmap_find(const hashmap_t hashmap, const int key);
queue_t hashmap_to_queue(const hashmap_t hashmap);
void hashmap_destroy(hashmap_t hashmap);

#endif
