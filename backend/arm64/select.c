//
// Created by Dimitar Anastassov on 21.11.26.
//

#include "select.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* nothing stays in these from one IR instruction to the next */
#define W0   m_reg(0, 0)     /* return value             */
#define W9   m_reg(9, 0)     /* left operand and result  */
#define W10  m_reg(10, 0)    /* right operand            */

static void select_func(MProgram* m, const IrFunc* f);
static void select_block(MFunc* mf, const IrBlock* b);
static void select_instr(MBlock* mb, const IrInstr* instr);

static void load_into(MBlock* mb, MOperand reg, Operand o);
static void store_from(MBlock* mb, MOperand reg, int vreg);
static AOp  arm_op(IrOp op);


MProgram* select_program(const IrProgram* p) {
    if (!p) {
        printf("select_program: invalid program\n");
        exit(1);
    }

    MProgram* m = m_program_new();

    for (const IrFunc* f = p->head; f; f = f->next) {
        select_func(m, f);
    }

    return m;
}

/* traversal */

/* slot N = %vN, so the function needs one slot per vreg */
static void select_func(MProgram* m, const IrFunc* f) {
    MFunc* mf = m_func_new(m, f->name, f->next_vreg);

    for (const IrBlock* b = f->head; b; b = b->next) {
        select_block(mf, b);
    }
}

static void select_block(MFunc* mf, const IrBlock* b) {
    MBlock* mb = m_block_new(mf, b->label);

    for (const IrInstr* instr = b->head; instr; instr = instr->next) {
        select_instr(mb, instr);
    }
}

static void select_instr(MBlock* mb, const IrInstr* instr) {
    switch (instr->op) {
        case IR_ADD: case IR_SUB: case IR_MUL: case IR_SDIV:
        case IR_AND: case IR_OR:  case IR_XOR: case IR_SHL: case IR_SHR:
            load_into(mb, W9, instr->binop.lhs);
            load_into(mb, W10, instr->binop.rhs);
            m_append(mb, m_binop(arm_op(instr->op), W9, W9, W10));
            store_from(mb, W9, instr->dst);
            break;

        /* %vN = alloca reserves memory, but the frame already has slot N for it,
         * so slot N IS the variable and nothing needs to happen at runtime */
        case IR_ALLOCA:
            break;

        case IR_LOAD:
            m_append(mb, m_ldr(W9, m_slot(instr->load.addr.vreg)));
            store_from(mb, W9, instr->dst);
            break;

        case IR_STORE:
            load_into(mb, W9, instr->store.value);
            m_append(mb, m_str(W9, m_slot(instr->store.addr.vreg)));
            break;

        case IR_RET:
            if (instr->ret.value.kind != OPD_NONE) {
                load_into(mb, W0, instr->ret.value);
            }

            m_append(mb, m_ret());
            break;

        default:
            printf("select: unsupported IR instruction %d\n", instr->op);
            exit(1);
    }
}

/* helpers */

/* vreg -> ldr from its slot,  constant -> mov, or movz + movk when it needs more than 16 bits */
static void load_into(MBlock* mb, MOperand reg, Operand o) {
    switch (o.kind) {
        case OPD_VREG:
            m_append(mb, m_ldr(reg, m_slot(o.vreg)));
            break;

        case OPD_IMM: {
            uint32_t u = (uint32_t)o.imm;   /* -5 -> 0xFFFFFFFB */

            if (u <= 0xFFFF) {
                m_append(mb, m_mov(reg, u));
            } else {
                m_append(mb, m_movz(reg, u & 0xFFFF));
                m_append(mb, m_movk(reg, u >> 16));
            }
            break;
        }

        case OPD_NONE:
            printf("select: operand is missing\n");
            exit(1);
    }
}

/* the result of an instruction goes to the slot of its vreg */
static void store_from(MBlock* mb, MOperand reg, int vreg) {
    m_append(mb, m_str(reg, m_slot(vreg)));
}

static AOp arm_op(IrOp op) {
    switch (op) {
        case IR_ADD:  return A_ADD;
        case IR_SUB:  return A_SUB;
        case IR_MUL:  return A_MUL;
        case IR_SDIV: return A_SDIV;
        case IR_AND:  return A_AND;
        case IR_OR:   return A_ORR;
        case IR_XOR:  return A_EOR;
        case IR_SHL:  return A_LSL;
        case IR_SHR:  return A_ASR;   /* the IR's shr is ashr: keeps the sign */

        default:
            printf("select: IR op %d is not a binop\n", op);
            exit(1);
    }
}
