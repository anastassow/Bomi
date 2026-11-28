//
// Created by Dimitar Anastassov on 28.09.26.
//

#ifndef BOMI_AST_H
#define BOMI_AST_H

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
    TYPE_INT
} TypeKind;

typedef enum {
    NODE_NUM = 0x00,
    NODE_BINOP = 0x01,
    NODE_NEG = 0x02,
    NODE_FUNC = 0x03,
    NODE_PROGRAM = 0x04,
    NODE_RETURN = 0x05,
    NODE_DECLARATION = 0x06,
    NODE_VAR = 0x07,
    NODE_ASSIGN = 0x08,
    NODE_EXPR_STMT = 0x09,
    NODE_UNARY = 0x10
} NodeType;

typedef struct { NodeType type; struct Node* next; } Node;

typedef struct { long value; }                                                          NumNode; // constNode
typedef struct { Op op; Node* l; Node* r; }                                             BinOpNode; // oprerationNode
typedef struct { char* name; struct LinkedList* body; TypeKind return_type;}            FuncNode; // int x(int z, int d) { } Decl -> decl func
typedef struct { struct LinkedList* program; }                                          ProgramNode; //
typedef struct { Node* expr; }                                                          ReturnNode;
typedef struct { TypeKind type; char* name; Node* init; }                               DeclNode; // int x; int x = 10;
typedef struct { char* name; }                                                          VarNode; // x += 4 -> x = x + 4
typedef struct { Op op; Node* target; Node* value; }                                    AssignNode;
typedef struct { Node* expr; }                                                          ExprStmtNode;
typedef struct { Op op; Node* operand; }                                                UnaryNode;

// new structures that are relevant for the grammar
typedef struct { Node base; struct LinkedList* declarations; }                                     TranslationUnitNode; //ready
typedef struct { Node base; struct LinkedList* specifiers; Node* declarator; Node* body; }         FunctionDefinitionNode; // i haven't made a declarator struct and specifiers
typedef struct { Node base; struct LinkedList* specifiers; struct LinkedList* init_declarators; }  DeclarationNode; // i haven't done specifiers
typedef struct { Node base; Node* declarator; Node* initializer; }                                 InitDeclaratorNode; // i haven't done declarator
typedef struct { Node base; struct LinkedList* items; }                                            InitializerListNode; // ready


// struct Node{
//     NodeType type;
//     Node*    next;
//     union {
//         NumNode         num;
//         BinOpNode       binop;
//         FuncNode        func;
//         ProgramNode     program;
//         ReturnNode      ret;
//         DeclNode        decl;
//         VarNode         var;
//         AssignNode      assign;
//         ExprStmtNode    expr_stmt;
//         UnaryNode       unary;
//     };
// };

Node* new_num(long value);
Node* new_binop(Op op, Node* left, Node* right);
Node* new_neg(Node* operand);
Node* new_func(char* name, struct LinkedList* body, TypeKind return_type);
Node* new_program(struct LinkedList* program);
Node* new_return(Node* expr);
Node* new_decl(TypeKind type, char* name, Node* init);
Node* new_var(char* name);
Node* new_assign(Op op, Node* target, Node* value);
Node* new_expr_stmt(Node* expr);
Node* new_unary(Op op, Node* operand);

void list_append(struct LinkedList* dst, struct LinkedList* src);

// for debug clearance
const char* op_name(Op op);
void print_tree(const Node *n, FILE *out);
void free_tree(Node* n);

#endif //BOMI_AST_H
