#ifndef HASHMAP_H
#define HASHMAP_H

#include "dtorr/structs.h"
#include "log.h"

dtorr_hashmap* hashmap_init(unsigned long size);

int hashmap_insert(dtorr_hashmap* map, char* key, dtorr_node* value);

dtorr_node* hashmap_get(dtorr_hashmap* map, char* key);

void hashmap_free(dtorr_hashmap* map);

dtorr_hashnode** hashmap_entries(dtorr_hashmap* map, char sort_by_key);

#endif