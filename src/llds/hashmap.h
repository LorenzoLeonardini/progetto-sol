#ifndef _HASHMAP_H
#define _HASHMAP_H

typedef struct hmap_bucket_element {
	int key;
	void *element;
	struct hmap_bucket_element *next;
};
typedef struct hmap_bucket_element *bucket_list_t;

typedef struct {
	int size;
	int count;
	bucket_list_t *arr;
} hmap_t;
typedef hmap_t *hashmap_t;

hashmap_t hashmap_create(int size);
void *hashmap_get(hashmap_t hmap, int key);
void hashmap_set(hashmap_t hmap, int key, void *value);

#endif
