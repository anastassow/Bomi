//
// Created by Dimitar Anastassov on 28.09.26.
//

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>

/* name -> symbol (helpers for debugging clearance) */

// list with reference name -> symbol (made for debug clearance)
static const char* const OP_NAMES[] = {
    [OP_ADD] = "+",   [OP_SUB] = "-",   [OP_MUL] = "*",
    [OP_DIV] = "/",   [OP_MOD] = "%",
    [OP_SHL] = "<<",  [OP_SHR] = ">>",
    [OP_LT]  = "<",   [OP_GT]  = ">",   [OP_LE] = "<=",  [OP_GE] = ">=",
    [OP_EQ]  = "==",  [OP_NE]  = "!=",
    [OP_BAND] = "&",  [OP_BXOR] = "^",  [OP_BOR] = "|",
    [OP_LAND] = "&&", [OP_LOR]  = "||",
    [OP_ASSIGN] = "=",       [OP_MUL_ASSIGN] = "*=",  [OP_DIV_ASSIGN] = "/=",
    [OP_MOD_ASSIGN] = "%=",  [OP_ADD_ASSIGN] = "+=",  [OP_SUB_ASSIGN] = "-=",
    [OP_SHL_ASSIGN] = "<<=", [OP_SHR_ASSIGN] = ">>=", [OP_AND_ASSIGN] = "&=",
    [OP_XOR_ASSIGN] = "^=",  [OP_OR_ASSIGN]  = "|=",
    [OP_COMMA] = ",",
    [OP_NEG] = "-x",  [OP_POS] = "+x",  [OP_BNOT] = "~",  [OP_LNOT] = "!",
    [OP_ADDR] = "&x", [OP_DEREF] = "*x",
    [OP_PRE_INC] = "++x",  [OP_PRE_DEC] = "--x",
    [OP_POST_INC] = "x++", [OP_POST_DEC] = "x--",
    [OP_SIZEOF] = "sizeof",
};

const char* op_name(Op op) {
    return OP_NAMES[op];
}

/* Functions for creating nodes */

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
    n->num.value = value;
    return n;
}

Node* new_binop(Op op, Node* left, Node* right) {
    Node* n = new_node(NODE_BINOP);
    n->binop.l = left;
    n->binop.r = right;
    n->binop.op = op;
    return n;
}

Node* new_func(char* name, LinkedList* body) {
    Node* n = new_node(NODE_FUNC);
    n->func.name = name;
    n->func.body = body;

    return n;
}

Node* new_program(LinkedList* program) {
      Node* n = new_node(NODE_PROGRAM);
      n->program.program = program;

      return n;
}

Node* new_return(Node* expr) {
    Node* n = new_node(NODE_RETURN);
    n->ret.expr = expr;

    return n;
}


/* Functions for debug clearance for printing */

static void print_rec(const Node *n, const char *prefix, int last, FILE *out);

static void print_list(const Node *n, const char *prefix, FILE *out) {
    for (; n; n = n->next)
        print_rec(n, prefix, n->next == NULL, out);
}

static void print_rec(const Node *n, const char *prefix, int last, FILE *out) {
    fprintf(out, "%s%s", prefix, last ? "└── " : "├── ");

    if (!n) {
        fprintf(out, "(null)\n");
        return;
    }

    switch (n->type) {
        case NODE_NUM:      fprintf(out, "%ld\n", n->num.value);            return;
        case NODE_NEG:      fprintf(out, "neg\n");                          break;
        case NODE_BINOP:    fprintf(out, "%s\n", op_name(n->binop.op));     break;
        case NODE_FUNC:     fprintf(out, "func %s\n", n->func.name);        break;
        case NODE_PROGRAM:  fprintf(out, "program\n");                      break;
        case NODE_RETURN:   fprintf(out, "return\n");                       break;

    }

    char child[256];
    snprintf(child, sizeof child, "%s%s", prefix, last ? "    " : "│   ");

    switch (n->type) {
        case NODE_NUM:
            break;
        case NODE_BINOP:
            print_rec(n->binop.l, child, 0, out);
            print_rec(n->binop.r, child, 1, out);
            break;
        case NODE_FUNC:
            if (n->func.body) print_list(n->func.body->head, child, out);
            break;
        case NODE_PROGRAM:
            if (n->program.program) print_list(n->program.program->head, child, out);
            break;
        case NODE_RETURN:
            if (n->ret.expr) print_list(n->ret.expr, child, out);
    }
}


void print_tree(const Node *n, FILE *out) { print_rec(n, "  ", 1, out); }

/* Function for cleaning memory  */

void free_tree(Node* n) {
    while (n) {
        Node* next = n->next;

        switch (n->type) {
            case NODE_NUM:
                break;
            case NODE_BINOP:
                free_tree(n->binop.l);
                free_tree(n->binop.r);
                break;
            case NODE_FUNC:
                free(n->func.name);
                list_free(n->func.body);
                break;
            case NODE_PROGRAM:
                list_free(n->program.program);
                break;
            case NODE_RETURN:
                free_tree(n->ret.expr);
                break;
        }

        free(n);
        n = next;
    }
}
