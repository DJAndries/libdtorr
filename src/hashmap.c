#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

static unsigned long hash(unsigned char *str) { /* djb2 */
  unsigned long hash = 5381;
  int c;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }

  return hash;
}

dtorr_hashmap* hashmap_init(unsigned long size) {
  dtorr_hashmap* result = (dtorr_hashmap*)malloc(sizeof(dtorr_hashmap));

  if (result == 0) {
    return 0;
  }

  result->elements = (dtorr_hashnode**)malloc(sizeof(dtorr_hashnode*) * size);
  memset(result->elements, 0, sizeof(dtorr_hashnode*) * size);

  if (result->elements == 0) {
    return 0;
  }
  result->map_size = size;
  result->entry_count = 0;

  return result;
}

int hashmap_insert(dtorr_hashmap* map, char* key, dtorr_node* value) {
  unsigned long index, i;
  dtorr_hashnode* hashnode;

  index = hash((unsigned char*)key) % map->map_size;

  for (i = 0; i < map->map_size; i++) {
    if (map->elements[index] == 0) {
      hashnode = (dtorr_hashnode*)malloc(sizeof(dtorr_hashnode));

      if (hashnode == 0) {
        return 1;
      }

      hashnode->key = key;
      hashnode->value = value;

      map->elements[index] = hashnode;
      map->entry_count++;
      return 0;
    }
    index = (index + 1) % map->map_size;
  }

  return 2;
}

dtorr_node* hashmap_get(dtorr_hashmap* map, char* key) {
  unsigned long index, i;
  dtorr_hashnode* element;
  index = hash((unsigned char*)key) % map->map_size;

  for (i = 0; i < map->map_size; i++) {
    element = map->elements[index];
    if (element != 0 && strcmp(element->key, key) == 0) {
      return element->value;
    }
  }

  return 0;
}

void hashmap_free(dtorr_hashmap* map) {
  free(map->elements);
  free(map);
}

static int key_sort(const void* a, const void* b) {
  dtorr_hashnode* a_node = *((dtorr_hashnode**)a);
  dtorr_hashnode* b_node = *((dtorr_hashnode**)b);
  return strcmp(a_node->key, b_node->key);
}

dtorr_hashnode** hashmap_entries(dtorr_hashmap* map, char sort_by_key) {
  unsigned long i;
  unsigned long result_index = 0;
  dtorr_hashnode* element;
  dtorr_hashnode** result = (dtorr_hashnode**)malloc(sizeof(dtorr_hashnode*) * map->entry_count);
  
  if (result == 0) {
    return 0;
  }
  
  for (i = 0; i < map->map_size && result_index < map->entry_count; i++) {
    element = map->elements[i];
    if (element != 0) {
      result[result_index++] = element;
    }
  }

  if (sort_by_key) {
    qsort(result, map->entry_count, sizeof(dtorr_hashnode*), &key_sort);
  }

  return result;
}