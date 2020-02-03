#ifndef LIST_H
#define LIST_H

#include "dtorr/structs.h"

int list_insert(dtorr_listnode** head, void* value);

void list_free(dtorr_listnode* head, char free_value);

void list_remove(dtorr_listnode** head, void* value);

#endif