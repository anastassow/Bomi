//
// Created by Dimitar Anastassov on 21.11.26.
//

#include "mir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* helpers */

static void* m_alloc(size_t size) {
    void* ptr = calloc(1, size);
    if (!ptr) {
        printf("Calloc failed at line %d in file: %s", __LINE__, __FILE__);
        exit(1);
    }

    return ptr;
}

static char* m_strdup(const char* s) {
    char* copy = m_alloc(strlen(s) + 1);
    strcpy(copy, s);

    return copy;
}

/* building the lists */

MProgram* m_program_new(void) {
    return m_alloc(sizeof(MProgram));
}

MFunc* m_func_new(MProgram* p, const char* name, int nslots) {
    if (!p || !name) {
        printf("m_func_new: invalid program or name\n");
        exit(1);
    }

    MFunc* f = m_alloc(sizeof(MFunc));
    f->name = m_strdup(name);
    f->nslots = nslots;
    f->frame_size = 0;

    if (!p->head) {
        p->head = f;
    } else {
        p->tail->next = f;
    }

    p->tail = f;

    return f;
}

MBlock* m_block_new(MFunc* f, const char* label) {
    if (!f || !label) {
        printf("m_block_new: invalid function or label\n");
        exit(1);
    }

    MBlock* b = m_alloc(sizeof(MBlock));
    b->label = m_strdup(label);

    if (!f->head) {
        f->head = b;
    } else {
        f->tail->next = b;
    }

    f->tail = b;

    return b;
}

/* making operands */

MOperand m_reg(int num, int is64) {
    if (num < 0 || num > REG_SP) {
        printf("m_reg: no register number %d\n", num);
        exit(1);
    }

    if (num == REG_SP && !is64) {
        printf("m_reg: sp is always 64-bit\n");
        exit(1);
    }

    MOperand op = {
        .kind = MO_REG,
        .reg = { .num = num, .is64 = is64 ? 1 : 0 }
    };

    return op;
}

MOperand m_imm(long value) {
    MOperand op = {
        .kind = MO_IMM,
        .imm = value
    };

    return op;
}

MOperand m_slot(int n) {
    if (n < 0) {
        printf("m_slot: negative slot %d\n", n);
        exit(1);
    }

    MOperand op = {
        .kind = MO_SLOT,
        .slot = n
    };

    return op;
}

MOperand m_mem(int off, MemMode mode) {
    MOperand op = {
        .kind = MO_MEM,
        .mem = { .off = off, .mode = mode }
    };

    return op;
}

/* making instructions */

/* operand checks */

static void need_reg(const char* fn, MOperand o) {
    if (o.kind != MO_REG) {
        printf("%s: expected a register\n", fn);
        exit(1);
    }
}

static void need_addr(const char* fn, MOperand o) {
    if (o.kind != MO_SLOT && o.kind != MO_MEM) {
        printf("%s: expected an address ([slot N] or [sp, ...])\n", fn);
        exit(1);
    }
}

static void need_imm16(const char* fn, long imm) {
    if (imm < 0 || imm > 0xFFFF) {
        printf("%s: immediate %ld does not fit in 16 bits (0 .. 65535)\n", fn, imm);
        exit(1);
    }
}

/* not linked into any block yet */
static MInstr* m_instr_new(AOp op, int nops, const MOperand ops[]) {
    MInstr* instr = m_alloc(sizeof(MInstr));
    instr->op = op;
    instr->nops = nops;

    for (int k = 0; k < nops; k++) {
        instr->ops[k] = ops[k];
    }

    return instr;
}

MInstr* m_mov(MOperand dst, long imm) {
    need_reg("m_mov", dst);
    need_imm16("m_mov", imm);

    MOperand ops[] = { dst, m_imm(imm) };
    return m_instr_new(A_MOV, 2, ops);
}

MInstr* m_movz(MOperand dst, long imm) {
    need_reg("m_movz", dst);
    need_imm16("m_movz", imm);

    MOperand ops[] = { dst, m_imm(imm) };
    return m_instr_new(A_MOVZ, 2, ops);
}

MInstr* m_movk(MOperand dst, long imm) {
    need_reg("m_movk", dst);
    need_imm16("m_movk", imm);

    MOperand ops[] = { dst, m_imm(imm) };
    return m_instr_new(A_MOVK, 2, ops);
}

MInstr* m_binop(AOp op, MOperand dst, MOperand lhs, MOperand rhs) {
    switch (op) {
        case A_ADD: case A_SUB: case A_MUL: case A_SDIV:
        case A_AND: case A_ORR: case A_EOR: case A_LSL: case A_ASR:
            break;

        default:
            printf("m_binop: instruction %d is not a binop\n", op);
            exit(1);
    }

    need_reg("m_binop", dst);
    need_reg("m_binop", lhs);

    if (rhs.kind == MO_IMM) {
        /* mul and sdiv have no #imm form, so only add / sub get one */
        if (op != A_ADD && op != A_SUB) {
            printf("m_binop: instruction %d needs a register, not #%ld\n", op, rhs.imm);
            exit(1);
        }

        if (rhs.imm < 0 || rhs.imm > 4095) {
            printf("m_binop: add / sub immediate %ld is not in 0 .. 4095\n", rhs.imm);
            exit(1);
        }
    } else {
        need_reg("m_binop", rhs);
    }

    MOperand ops[] = { dst, lhs, rhs };
    return m_instr_new(op, 3, ops);
}

MInstr* m_msub(MOperand dst, MOperand n, MOperand m, MOperand a) {
    need_reg("m_msub", dst);
    need_reg("m_msub", n);
    need_reg("m_msub", m);
    need_reg("m_msub", a);

    MOperand ops[] = { dst, n, m, a };
    return m_instr_new(A_MSUB, 4, ops);
}

MInstr* m_ldr(MOperand dst, MOperand addr) {
    need_reg("m_ldr", dst);
    need_addr("m_ldr", addr);

    MOperand ops[] = { dst, addr };
    return m_instr_new(A_LDR, 2, ops);
}

MInstr* m_str(MOperand src, MOperand addr) {
    need_reg("m_str", src);
    need_addr("m_str", addr);

    MOperand ops[] = { src, addr };
    return m_instr_new(A_STR, 2, ops);
}

MInstr* m_stp(MOperand r1, MOperand r2, MOperand addr) {
    need_reg("m_stp", r1);
    need_reg("m_stp", r2);
    need_addr("m_stp", addr);

    MOperand ops[] = { r1, r2, addr };
    return m_instr_new(A_STP, 3, ops);
}

MInstr* m_ldp(MOperand r1, MOperand r2, MOperand addr) {
    need_reg("m_ldp", r1);
    need_reg("m_ldp", r2);
    need_addr("m_ldp", addr);

    MOperand ops[] = { r1, r2, addr };
    return m_instr_new(A_LDP, 3, ops);
}

MInstr* m_ret(void) {
    return m_instr_new(A_RET, 0, NULL);
}

void m_program_free(MProgram* p) {
    if (!p) return;

    MFunc* f = p->head;

    while (f) {
        MFunc* next_f = f->next;

        MBlock* b = f->head;

        while (b) {
            MBlock* next_b = b->next;

            MInstr* instr = b->head;

            while (instr) {
                MInstr* next_instr = instr->next;

                free(instr);

                instr = next_instr;
            }

            free(b->label);
            free(b);

            b = next_b;
        }

        free(f->name);
        free(f);

        f = next_f;
    }

    free(p);
}
