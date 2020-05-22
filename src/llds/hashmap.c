#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

#include "hashmap.h"

/**
 * This is an extremely simple hashmap implementation. It's made for the SOL
 * project and therefore does not require resizing nor complicated hash
 * functions. This should not be used for any other project.
 */

static int hash(const int key, const int size);
static int compare(const void *a, const void *b);

hashmap_t hashmap_create(const int size) {
	if(size <= 0) return NULL;
	hashmap_t hashmap = (hashmap_t) malloc(sizeof(hashmap_struct_t));
	hashmap->size = size;
	hashmap->element_count = 0;
	hashmap->arr = (queue_t*) malloc(sizeof(queue_t) * size);
	for(int i = 0; i < size; i++)
		hashmap->arr[i] = queue_create();
	return hashmap;
}

void hashmap_add(hashmap_t hashmap, const int key, void *element) {
	hashmap_element_t *e = (hashmap_element_t*) malloc(sizeof(hashmap_element_t));
	e->key = key;
	e->element = element;
	int h = hash(key, hashmap->size);
	queue_enqueue(hashmap->arr[h], e);
	hashmap->element_count++;
}

void *hashmap_remove(hashmap_t hashmap, const int key) {
	int h = hash(key, hashmap->size);
	// Uses queue remove function
	void *res = queue_remove_cmp(hashmap->arr[h], &key, compare);
	if (res == NULL) return NULL;

	void *e = ((hashmap_element_t*)res)->element;
	free(res);
	hashmap->element_count--;
	return e;
}

void *hashmap_find(const hashmap_t hashmap, const int key) {
	int h = hash(key, hashmap->size);
	queue_element_t *e = hashmap->arr[h]->head;
	while(e != NULL) {
		if(compare(e, &key) == 0) {
			return ((hashmap_element_t*)e->element)->element;
		}
		e = e->next;
	}
	return NULL;
}

queue_t hashmap_to_queue(const hashmap_t hashmap) {
	queue_t queue = queue_create();
	for(int i = 0; i < hashmap->size; i++) {
		queue_element_t *e = hashmap->arr[i]->head;
		while(e != NULL) {
			queue_enqueue(queue, ((hashmap_element_t*)e->element)->element);
			e = e->next;
		}
	}
	return queue;
}

void hashmap_destroy(hashmap_t hashmap) {
	if(hashmap->element_count != 0)
		fprintf(stderr, "Hashmap is being destroyed but it's not empty. "
		"Memory leaks are inevitable\n");
	for(int i = 0; i < hashmap->size; i++)
		queue_destroy(hashmap->arr[i]);
	free(hashmap->arr);
	free(hashmap);
}

static int hash(const int key, const int size) {
	return key % size;
}

static int compare(const void *a, const void *b) {
	return ((hashmap_element_t*)a)->key - *((int*)b);
}
