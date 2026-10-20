//
// Created by Dimitar Anastassov on 28.09.26.
//

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>

static Node* new_node(NodeType type) {
    Node* n = calloc(1, sizeof(Node));
    if (!n) {
        printf("Allocation failed! Calloc row %d. File %s", __LINE__, __FILE__);
        exit(1);
    }
    n->type = type;

    return n;
}

Node* new_num(long value) {
    Node* n = new_node(NODE_NUM);
    n->value = value;
    return n;
}

Node* new_binop(Op op, Node* left, Node* right) {
    Node* n = new_node(NODE_BINOP);
    n->left = left;
    n->right = right;
    n->op = op;
    return n;
}
Node* new_neg(Node* operand) {
    Node* n = new_node(NODE_NEG);
    n->left = operand;

    return n;
}

static void print_rec(const Node *n, const char *prefix, int last, FILE *out) {
    fprintf(out, "%s%s", prefix, last ? "└── " : "├── ");
    switch (n->type) {
        case NODE_NUM:   fprintf(out, "%ld\n", n->value); return;
        case NODE_NEG:   fprintf(out, "neg\n");           break;
        case NODE_BINOP: fprintf(out, "%c\n", n->op);     break;
    }
    char child[256];
    snprintf(child, sizeof child, "%s%s", prefix, last ? "    " : "│   ");
    if (n->type == NODE_NEG) print_rec(n->left, child, 1, out);
    else { print_rec(n->left, child, 0, out); print_rec(n->right, child, 1, out); }
}

void print_tree(const Node *n, FILE *out) { print_rec(n, "  ", 1, out); }

void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->left);
    free_tree(n->right);
    free(n);
}