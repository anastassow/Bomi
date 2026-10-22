//
// Created by Dimitar Anastassov on 28.09.26.
//

#ifndef CC_AST_H
#define CC_AST_H

#include <stdio.h>

#include "data_structures/linkedList.h"

typedef enum {
    /* arithmetic */
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    /* shift */
    OP_SHL, OP_SHR,
    /* relational */
    OP_LT, OP_GT, OP_LE, OP_GE,
    /* equality */
    OP_EQ, OP_NE,
    /* bitwise */
    OP_BAND, OP_BXOR, OP_BOR,
    /* logical */
    OP_LAND, OP_LOR,
    /* assignment */
    OP_ASSIGN, OP_MUL_ASSIGN, OP_DIV_ASSIGN, OP_MOD_ASSIGN, OP_ADD_ASSIGN,
    OP_SUB_ASSIGN, OP_SHL_ASSIGN, OP_SHR_ASSIGN, OP_AND_ASSIGN,
    OP_XOR_ASSIGN, OP_OR_ASSIGN,
    /* comma */
    OP_COMMA,
    /* unary */
    OP_NEG, OP_POS, OP_BNOT, OP_LNOT, OP_ADDR, OP_DEREF,
    OP_PRE_INC, OP_PRE_DEC, OP_POST_INC, OP_POST_DEC, OP_SIZEOF,
} Op;


typedef enum {
    NODE_NUM = 0x00,
    NODE_BINOP = 0x01,
    NODE_NEG = 0x02,
    NODE_FUNC = 0x03,
    NODE_PROGRAM = 0x04,
} NodeType;

typedef struct Node Node;

typedef struct { long value; }                                  NumNode;
typedef struct { Op op; Node* l; Node* r; }                     BinOpNode;
typedef struct { char* name; struct LinkedList* body; }         FuncNode;
typedef struct { struct LinkedList* program; }                         ProgramNode;

struct Node{
    NodeType type;
    Node*    next;
    union {
        NumNode         num;
        BinOpNode       binop;
        FuncNode        func;
        ProgramNode     program;
    };
};

Node* new_num(long value);
Node* new_binop(Op op, Node* left, Node* right);
Node* new_neg(Node* operand);
Node* new_func(char* name, struct LinkedList* body);
Node* new_program(struct LinkedList* program);

// for debug clearance
void print_tree(const Node *n, FILE *out);
void free_tree(Node* n);

#endif //CC_AST_H
