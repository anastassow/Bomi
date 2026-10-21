//
// Created by Dimitar Anastassov on 21.10.26.
//

#ifndef CC_LINKEDLIST_H
#define CC_LINKEDLIST_H

#include "ast.h"

typedef struct LinkedList {
    struct Node* head;
    struct Node* tail;
} LinkedList;

LinkedList* list_new(void);
void list_push(LinkedList* list, struct Node* item);

void list_free(LinkedList* list);

#endif //CC_LINKEDLIST_H
