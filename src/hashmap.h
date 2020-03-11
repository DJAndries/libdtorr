#ifndef HASHMAP_H
#define HASHMAP_H

#include "dtorr/structs.h"
#include "log.h"

#define DEFAULT_HASHMAP_SIZE 512

dtorr_hashmap* hashmap_init(unsigned long long size);

int hashmap_insert(dtorr_hashmap* map, char* key, void* value);

void* hashmap_get(dtorr_hashmap* map, char* key);

void hashmap_free(dtorr_hashmap* map);

dtorr_hashnode** hashmap_entries(dtorr_hashmap* map, char sort_by_key);

#endif
