//
// Created by Dimitar Anastassov on 21.11.26.
//

#ifndef BOMI_MIR_H
#define BOMI_MIR_H

enum { REG_SP = 31 };   /* 0 .. 30 are w0..w30 / x0..x30 */

typedef enum { MO_REG, MO_IMM, MO_SLOT, MO_MEM } MOpKind;

typedef enum {
    MEM_OFF,        /* [sp, #off]                       */
    MEM_PRE,        /* [sp, #off]!   sp += off before   */
    MEM_POST        /* [sp], #off    sp += off after    */
} MemMode;

typedef struct { int num; int is64; }    MRegData;  /* w9 = {9, 0}   x29 = {29, 1} */
typedef struct { int off; MemMode mode; } MMemData;  /* always relative to sp       */

typedef struct {
    MOpKind kind;
    union {
        MRegData reg;
        long     imm;
        int      slot;  /* [slot N], frame layout turns it into a mem */
        MMemData mem;
    };
} MOperand;

typedef enum {
    /* constants */
    A_MOV, A_MOVZ, A_MOVK,
    /* arithmetic */
    A_ADD, A_SUB, A_MUL, A_SDIV, A_MSUB,
    /* bitwise */
    A_AND, A_ORR, A_EOR, A_LSL, A_ASR,
    /* memory */
    A_LDR, A_STR, A_STP, A_LDP,
    A_RET
} AOp;

typedef struct MInstr {
    AOp             op;
    MOperand        ops[4];     /* msub has 4, the most ( for now for simplicity ) */
    int             nops;
    struct MInstr*  prev;
    struct MInstr*  next;
} MInstr;

typedef struct MBlock {
    char*           label;
    MInstr*         head, *tail;
    struct MBlock*  next;
} MBlock;

typedef struct MFunc {
    char*           name;
    int             nslots;     /* = IrFunc.next_vreg  */
    int             frame_size; /* set by frame_layout */
    MBlock*         head, *tail;
    struct MFunc*   next;
} MFunc;

typedef struct {
    MFunc*          head, *tail;
} MProgram;

#endif //BOMI_MIR_H
