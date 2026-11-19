//
// Created by Dimitar Anastassov on 29.09.26.
//

#ifndef BOMI_LOWER_H
#define BOMI_LOWER_H

#include "frontend/ast.h"
#include "ir/ir.h"

/* a local variable: its name and the stack slot ir_alloca gave it */
typedef struct Symbol {
    const char*    name;   /* borrowed from the AST node, not owned */
    Operand        addr;   /* %vN returned by ir_alloca */
    struct Symbol* next;
} Symbol;

typedef struct {
    IrProgram* program;
    IrFunc* current_func;
    IrBlock* current_block;
    Symbol* locals;        /* variables of current_func, newest first */
} LoweringContext;

IrProgram* lower_program(const Node* program_root);

#endif //BOMI_LOWER_H
