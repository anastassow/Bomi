//
// Created by Dimitar Anastassov on 29.09.26.
//

#ifndef BOMI_LOWER_H
#define BOMI_LOWER_H

#include "frontend/ast.h"
#include "ir/ir.h"

Operand lower_expr(IrProgram* p, const Node* n);

#endif //BOMI_LOWER_H
