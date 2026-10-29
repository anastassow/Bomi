//
// Created by Dimitar Anastassov on 29.09.26.
//

#ifndef BOMI_IR_H
#define BOMI_IR_H

#include <stdio.h>

typedef enum { OPD_NONE, OPD_IMM, OPD_VREG } OpdKind;

typedef struct {
    OpdKind kind;
    union { long imm; int vreg; };
} Operand;

typedef enum {
    /* arithmetic:  dst = a op b */
    IR_ADD, IR_SUB, IR_MUL, IR_SDIV, IR_SREM,
    /* bitwise:     dst = a op b */
    IR_AND, IR_OR, IR_XOR, IR_SHL, IR_SHR,
    /* terminators: end a block, no dst */
    IR_RET,          /* ret a        (a may be OPD_NONE) */
    /* later: IR_JMP, IR_BR, IR_CMP, IR_CALL, IR_LOAD, IR_STORE */
} IrOp;

typedef struct IrInstr {
    IrOp            op;
    int             dst; // destination for exmaple -> %v1 = add i64 %v0, %v1, %v1 is the dest
    Operand         a, b;
    struct IrInstr* next;
} IrInstr;

typedef struct IrBlock {
    char*           label;
    IrInstr*        head, *tail;
    struct IrBlock* next;
} IrBlock;

typedef struct IrFunc {
    char*           name;
    IrBlock*        head, *tail;
    int             next_vreg;
    int             next_label;   /* for unique block names */
    struct IrFunc*  next;
} IrFunc;

typedef struct {
    IrFunc*         head, *tail;
} IrProgram;

IrProgram* ir_program_new(void);
IrFunc* ir_func_new(IrProgram* p, const char* name);
IrBlock* ir_block_new(IrFunc* f, const char* label);

Operand ir_imm(long value);
Operand ir_none(void);
Operand ir_vreg(int vreg_next);

Operand ir_binop(IrFunc* f, IrBlock* b, IrOp op, Operand l, Operand r);
void ir_ret(IrBlock* b, Operand v);

void ir_print(const IrProgram* p, FILE* out);

void ir_program_free(IrProgram* p);

#endif //BOMI_IR_H
