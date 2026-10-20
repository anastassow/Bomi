//
// Created by Dimitar Anastassov on 29.09.26.
//

#include "lower.h"

#include <stdlib.h>

static IrOp to_ir_op(char op) {
    switch (op) {
        case '+': return IR_ADD;
        case '-': return IR_SUB;
        case '*': return IR_MUL;
        case '/': return IR_SDIV;
        case '%': return IR_MOD;
        default: ;
    }
    abort();
}

Operand lower_expr(IrProgram* p, const Node* n) {
    switch (n->type) {
        default: ;
        case NODE_NUM:
            return ir_imm(n->value);

        case NODE_NEG:
            return ir_neg(p, lower_expr(p, n->left));

        case NODE_BINOP:
            Operand l = lower_expr(p, n->left);
            Operand r = lower_expr(p, n->right);
            return ir_binop(p, to_ir_op(n->op), l, r);
    }
    abort();
}