//
// Created by Dimitar Anastassov on 21.10.26.
//

#include "data_structures/linkedList.h"
#include "frontend/ast.h"

#include <stdlib.h>

LinkedList* list_new(void) {
    LinkedList* list = (LinkedList*) calloc(1, sizeof(LinkedList));

    return list;
}

void list_push(LinkedList* list, Node* item) {
    if (!item) return;
    if (list->tail) list->tail->next = item;
    else            list->head = item;
    list->tail = item;
}

void list_free(LinkedList* list) {
    if (!list) return;
    free_tree(list->head);
    free(list);
}
