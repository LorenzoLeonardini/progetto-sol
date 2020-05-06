#include <stdlib.h>

#include "hashmap.h"

hashmap_t hashmap_create(int size) {
	if (size <= 0) return NULL;
	hashmap_t hmap = (hashmap_t) malloc(sizeof(hmap_t));
	hmap->size = size;
	hmap->count = 0;
	hmap->arr = (bucket_list_t*) malloc(sizeof(bucket_list_t) * size);
	return hmap;
}

static int hash(hashmap_t hmap, int key) {
	return key % hmap->size;
}

void *hashmap_get(hashmap_t hmap, int key) {
	int index = hash(hmap, key);
	bucket_list_t current = hmap->arr[index];
	while(current != NULL) {
		if(current->key == key) {
			return current->element;
		}
	}
	return NULL;
}

void hashmap_set(hashmap_t hmap, int key, void *value) {
	int index = hash(hmap, key);
	bucket_list_t element = (bucket_list_t) malloc(sizeof(struct hmap_bucket_element));
	element->key = key;
	element->element = value;
	element->next = NULL;
	if(hmap->arr[index] == NULL) {
		hmap->arr[index] = element;
		return;
	}
	bucket_list_t current = hmap->arr[index];
	while(current->next != NULL) {
		current = current->next;
	}
	current->next = element;
}
