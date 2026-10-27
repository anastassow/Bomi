//
// Created by Dimitar Anastassov on 29.09.26.
//

#ifndef BOMI_IR_H
#define BOMI_IR_H

#include <stdio.h>

typedef enum { OPD_VREG, OPD_IMM, OPD_NONE} OpdKind;

typedef struct {
    OpdKind kind;
    union { int vreg; long imm; };
} Operand;

typedef enum { IR_ADD, IR_SUB, IR_MUL, IR_SDIV, IR_NEG, IR_PRINT, IR_MOD} IrOp;

typedef struct Instr{
    IrOp          op;
    int           dst;
    Operand       a, b;
    struct Instr* next;
} Instr;

typedef struct {
    Instr* head, *tail;
    int    next_vreg;
} IrProgram;

IrProgram* ir_program_new(void);
void       ir_program_free(IrProgram *p);

Operand ir_imm(long value);
Operand ir_vreg(int n);
Operand ir_none(void);

Operand ir_binop(IrProgram *p, IrOp op, Operand l, Operand r);
Operand ir_neg(IrProgram *p, Operand v);
void    ir_print_val(IrProgram *p, Operand v);

void    ir_print(const IrProgram *p, FILE *out);

#endif //BOMI_IR_H
