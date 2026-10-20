//
// Created by Dimitar Anastassov on 28.09.26.
//

#ifndef CC_AST_H
#define CC_AST_H

#include <stdio.h>

typedef enum {
    /* arithmetic */ //ready
    OP_ADD        = '+',
    OP_SUB        = '-',
    OP_MUL        = '*',
    OP_DIV        = '/',
    OP_MOD        = '%',

    /* shift */
    OP_SHL        = '<<',
    OP_SHR        = '>>',

    /* relational */
    OP_LT         = '<',
    OP_GT         = '>',
    OP_LE         = '<=',
    OP_GE         = '>=',

    /* equality */
    OP_EQ         = '==',
    OP_NE         = '!=',

    /* bitwise */ // ready
    OP_BAND       = '&',
    OP_BXOR       = '^',
    OP_BOR        = '|',

    /* logical (short-circuit) */
    OP_LAND       = '&&',
    OP_LOR        = '||',

    /* assignment */
    OP_ASSIGN     = '=',
    OP_MUL_ASSIGN = '*=',
    OP_DIV_ASSIGN = '/=',
    OP_MOD_ASSIGN = '%=',
    OP_ADD_ASSIGN = '+=',
    OP_SUB_ASSIGN = '-=',
    OP_SHL_ASSIGN = '<<=',
    OP_SHR_ASSIGN = '>>=',
    OP_AND_ASSIGN = '&=',
    OP_XOR_ASSIGN = '^=',
    OP_OR_ASSIGN  = '|=',

    /* comma */
    OP_COMMA      = ',',

    /* unary */
    OP_NEG        = '-x',
    OP_POS        = '+x',
    OP_BNOT       = '~x',
    OP_LNOT       = '!x',
    OP_ADDR       = '&x',
    OP_DEREF      = '*x',
    OP_PRE_INC    = '++x',
    OP_PRE_DEC    = '--x',
    OP_POST_INC   = 'x++',
    OP_POST_DEC   = 'x--',
    OP_SIZEOF     = 'szof',
} Op;

typedef enum {
    NODE_NUM = 0x00,
    NODE_BINOP = 0x01,
    NODE_NEG = 0x02,
} NodeType;

typedef struct Node{
    NodeType     type;
    long         value;
    Op           op;
    struct Node* left;
    struct Node* right;
} Node;

Node* new_num(long value);
Node* new_binop(Op op, Node* left, Node* right);
Node* new_neg(Node* operand);

void print_tree(const Node *n, FILE *out);
void free_tree(Node* n);

#endif //CC_AST_H
