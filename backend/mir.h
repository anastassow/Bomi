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

/* lists */

MProgram* m_program_new(void);
MFunc*    m_func_new(MProgram* p, const char* name, int nslots);
MBlock*   m_block_new(MFunc* f, const char* label);

/* operands */

MOperand  m_reg(int num, int is64);    /* m_reg(9, 0) = w9,  m_reg(REG_SP, 1) = sp */
MOperand  m_imm(long value);
MOperand  m_slot(int n);
MOperand  m_mem(int off, MemMode mode);

/* instructions: these only make one, m_append / m_insert_before put it in a block */

MInstr*   m_mov  (MOperand dst, long imm);      /* imm 0 .. 65535            */
MInstr*   m_movz (MOperand dst, long imm);      /* low 16 bits, rest zeroed  */
MInstr*   m_movk (MOperand dst, long imm);      /* high 16 bits, lsl #16     */

/* add sub mul sdiv and orr eor lsl asr.
 * rhs can be #0 .. 4095 only for add / sub, otherwise a register */
MInstr*   m_binop(AOp op, MOperand dst, MOperand lhs, MOperand rhs);
MInstr*   m_msub (MOperand dst, MOperand n, MOperand m, MOperand a);   /* dst = a - n * m */

/* addr is m_slot(...) or m_mem(...) */
MInstr*   m_ldr  (MOperand dst, MOperand addr);
MInstr*   m_str  (MOperand src, MOperand addr);
MInstr*   m_stp  (MOperand r1, MOperand r2, MOperand addr);
MInstr*   m_ldp  (MOperand r1, MOperand r2, MOperand addr);
MInstr*   m_ret  (void);

/* placing */

MInstr*   m_append(MBlock* b, MInstr* instr);
MInstr*   m_insert_before(MBlock* b, MInstr* pos, MInstr* instr);  /* pos NULL = append */

void      m_program_free(MProgram* p);

#endif //BOMI_MIR_H
