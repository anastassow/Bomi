//
// Created by Dimitar Anastassov on 21.10.26.
//

#ifndef BOMI_LINKEDLIST_H
#define BOMI_LINKEDLIST_H

#include "frontend/ast.h"

typedef struct LinkedList {
    struct Node* head;
    struct Node* tail;
} LinkedList;

LinkedList* list_new(void);
void list_push(LinkedList* list, struct Node* item);

void list_free(LinkedList* list);

#endif //BOMI_LINKEDLIST_H
