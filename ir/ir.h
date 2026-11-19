//
// Created by Dimitar Anastassov on 29.09.26.
//

#ifndef BOMI_IR_H
#define BOMI_IR_H

#include <stdio.h>

/* IR types (kept separate from the AST's TypeKind) */
typedef enum { IR_VOID, IR_I32 } IrType;

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
    IR_RET,          /* ret value    (value may be OPD_NONE) */
    /* memory */
    IR_ALLOCA, IR_LOAD, IR_STORE
} IrOp;

typedef struct { Operand lhs, rhs; }       IrBinopData;   /* IR_ADD ... IR_SHR */
typedef struct { IrType type; }            IrAllocaData;  /* IR_ALLOCA */
typedef struct { Operand addr; }           IrLoadData;    /* IR_LOAD */
typedef struct { Operand value, addr; }    IrStoreData;   /* IR_STORE */
typedef struct { Operand value; }          IrRetData;     /* IR_RET */

typedef struct IrInstr {
    IrOp            op;        /* tag: which member of the union is valid */
    IrType          type;
    int             dst;       /* -1 if no result */
    struct IrInstr* next;
    union {
        IrBinopData  binop;
        IrAllocaData alloca;
        IrLoadData   load;
        IrStoreData  store;
        IrRetData    ret;
    };
} IrInstr;

typedef struct IrBlock {
    char*           label;
    IrInstr*        head, *tail;
    struct IrBlock* next;
} IrBlock;

typedef struct IrFunc {
    char*           name;
    IrType          ret_type;
    IrBlock*        head, *tail;
    int             next_vreg;
    int             next_label;   /* for unique block names */
    struct IrFunc*  next;
} IrFunc;

typedef struct {
    IrFunc*         head, *tail;
} IrProgram;

IrProgram* ir_program_new(void);
IrFunc* ir_func_new(IrProgram* p, const char* name, IrType ret_type);
IrBlock* ir_block_new(IrFunc* f, const char* label);
int ir_block_terminated(const IrBlock* b);

Operand ir_imm(long value);
Operand ir_none(void);
Operand ir_vreg(int vreg_next);

Operand ir_binop(IrFunc* f, IrBlock* b, IrOp op, Operand l, Operand r);
void    ir_ret(IrBlock* b, Operand v);
Operand ir_alloca(IrFunc* f, IrBlock* b);
Operand ir_load(IrFunc* f, IrBlock* b, Operand addr);
void    ir_store(IrBlock* b, Operand value, Operand addr);

void ir_print(const IrProgram* p, FILE* out);

void ir_program_free(IrProgram* p);

#endif //BOMI_IR_H
