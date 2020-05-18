#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

#include "hashmap.h"

static int hash(int key, int size);
static int compare(void *a, void *b);

hashmap_t hashmap_create(int size) {
	if(size <= 0) return NULL;
	hashmap_t hashmap = (hashmap_t) malloc(sizeof(hashmap_struct_t));
	hashmap->size = size;
	hashmap->element_count = 0;
	hashmap->arr = (queue_t*) malloc(sizeof(queue_t) * size);
	for(int i = 0; i < size; i++)
		hashmap->arr[i] = queue_create();
	return hashmap;
}

void hashmap_add(hashmap_t hashmap, int key, void *element) {
	hashmap_element_t *e = (hashmap_element_t*) malloc(sizeof(hashmap_element_t));
	e->key = key;
	e->element = element;
	int h = hash(key, hashmap->size);
	queue_add(hashmap->arr[h], e);
	hashmap->element_count++;
}

void *hashmap_remove(hashmap_t hashmap, int key) {
	int h = hash(key, hashmap->size);
	void *res = queue_remove_cmp(hashmap->arr[h], &key, compare);
	if (res != NULL) {
		void *e = ((hashmap_element_t*)res)->element;
		free(res);
		res = e;
		hashmap->element_count--;
	}
	return res;
}

void *hashmap_find(hashmap_t hashmap, int key) {
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

queue_t hashmap_to_queue(hashmap_t hashmap) {
	queue_t queue = queue_create();
	for(int i = 0; i < hashmap->size; i++) {
		queue_element_t *e = hashmap->arr[i]->head;
		while(e != NULL) {
			queue_add(queue, ((hashmap_element_t*)e->element)->element);
			e = e->next;
		}
	}
	return queue;
}

void hashmap_destroy(hashmap_t hashmap) {
	if(hashmap->element_count != 0)
		fprintf(stderr, "Hashmap is being destroyed but it's not empty. Memory leaks are inevitable\n");
	for(int i = 0; i < hashmap->size; i++)
		queue_destroy(hashmap->arr[i]);
	free(hashmap->arr);
	free(hashmap);
}

static int hash(int key, int size) {
	return key % size;
}

static int compare(void *a, void *b) {
	return ((hashmap_element_t*)a)->key - *((int*)b);
}
