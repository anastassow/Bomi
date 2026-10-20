//
// Created by Dimitar Anastassov on 28.09.26.
//

#ifndef CC_AST_H
#define CC_AST_H

#include <stdio.h>

typedef enum {
    NODE_NUM = 0x00,
    NODE_BINOP = 0x01,
    NODE_NEG = 0x02,
} NodeType;

typedef struct Node{
    NodeType     type;
    long         value;
    char         op;
    struct Node* left;
    struct Node* right;
} Node;

Node* new_num(long value);
Node* new_binop(char op, Node* left, Node* right);
Node* new_neg(Node* operand);

void print_tree(const Node *n, FILE *out);
void free_tree(Node* n);

#endif //CC_AST_H
