#ifndef STACK_H
#define STACK_H

#include "dtorr/structs.h"

struct stack {
  dtorr_node** elements;

  unsigned long long next_index;
  unsigned long long size;
};
typedef struct stack stack;

stack* stack_init(unsigned long long size);

int stack_push(stack* st, dtorr_node* node);

dtorr_node* stack_pop(stack* st);

dtorr_node* stack_peek(stack* st);

void stack_free(stack* st);

#endif
