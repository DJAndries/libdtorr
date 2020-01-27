#include "stack.h"
#include <stdlib.h>

stack* stack_init(unsigned long size) {
  stack* result = (stack*)malloc(stack);
  result->elements = (dtorr_node**)malloc(sizeof(dtorr_node*));
  result->size = size;
  result->next_index = 0;
  return result;
}

int stack_push(stack* st, dtorr_node* node) {
  if (st->next_index >= st->size) {
    return 1;
  }

  st->elements[st->next_index++] = node; 
}

dtorr_node* stack_pop(stack* st) {
  if (st->next_index == 0) {
    return 1;
  }

  return st->elements[--st->next_index];
}

dtorr_node* stack_peek(stack* st) {
  if (st->next_index == 0) {
    return 1;
  }

  return st->elements[st->next_index - 1];
}