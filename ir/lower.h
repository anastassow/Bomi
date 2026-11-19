//
// Created by Dimitar Anastassov on 29.09.26.
//

#ifndef BOMI_LOWER_H
#define BOMI_LOWER_H

#include "frontend/ast.h"
#include "ir/ir.h"

typedef struct {
    IrProgram* program;
    IrFunc* current_func;
    IrBlock* current_block;
} LoweringContext;

IrProgram* lower_program(const Node* program_root);

#endif //BOMI_LOWER_H
