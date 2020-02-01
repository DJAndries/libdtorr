#include "list.h"
#include <stdlib.h>

int list_insert(dtorr_listnode** head, void* value) {
  dtorr_listnode* node = (dtorr_listnode*)malloc(sizeof(dtorr_listnode));
  if (node == 0) {
    return 1;
  }
  node->value = value;
  node->next = *head;
  *head = node;
  return 0;
}

void list_remove(dtorr_listnode** head, void* value) {
  dtorr_listnode* prev = 0;
  dtorr_listnode** next;
  dtorr_listnode** it;
  for (it = head; *it != 0; it = next) {
    next = &(*it)->next;
    if ((*it)->value == value) {
      if (prev != 0) {
        prev->next = (*it)->next;
      }
      if (head == it) {
        *head = (*it)->next;
      }
      free(*it);
    } else {
      prev = *it;
    }
  }
}

void list_free(dtorr_listnode* head) {
  dtorr_listnode* it;
  dtorr_listnode* next;
  for (it = head; it != 0; it = next) {
    next = it->next;
    free(it);
  }
}