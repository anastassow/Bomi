//
// Created by Dimitar Anastassov on 21.11.26.
//

#include "frame.h"

#include <stdio.h>
#include <stdlib.h>

/* x up to the next multiple of align (align must be a power of 2):  ROUND_UP(8, 16) = 16 */
#define ROUND_UP(x, align)  (((x) + (align) - 1) & ~((align) - 1))

#define SLOT_SIZE     4       /* every slot holds one i32                  */
#define STACK_ALIGN   16      /* sp must always be a multiple of 16        */
#define FP_LR_SIZE    16      /* x29 + x30, saved by the prologue          */
#define MAX_SUB_IMM   4095    /* the biggest #imm of one  sub sp, sp, #imm */

#define X29  m_reg(29, 1)     /* frame pointer  */
#define X30  m_reg(30, 1)     /* return address */
#define SP   m_reg(REG_SP, 1)

static void layout_func(MFunc* f);
static void slots_to_sp(MFunc* f);
static void add_prologue(MFunc* f);
static void add_epilogues(MFunc* f);


void frame_layout(MProgram* m) {
    if (!m) {
        printf("frame_layout: invalid program\n");
        exit(1);
    }

    for (MFunc* f = m->head; f; f = f->next) {
        layout_func(f);
    }
}

static void layout_func(MFunc* f) {
    if (!f->head) {
        printf("frame_layout: function '%s' has no blocks\n", f->name);
        exit(1);
    }

    f->frame_size = ROUND_UP(f->nslots * SLOT_SIZE, STACK_ALIGN);

    if (f->frame_size > MAX_SUB_IMM) {
        printf("frame_layout: function '%s' needs %d bytes of stack, at most %d are supported\n",
               f->name, f->frame_size, MAX_SUB_IMM);
        exit(1);
    }

    slots_to_sp(f);
    add_prologue(f);
    add_epilogues(f);
}

/* sp does not move inside the body, so every slot is at a fixed offset from it */
static void slots_to_sp(MFunc* f) {
    for (MBlock* b = f->head; b; b = b->next) {
        for (MInstr* instr = b->head; instr; instr = instr->next) {
            for (int k = 0; k < instr->nops; k++) {
                MOperand* op = &instr->ops[k];

                if (op->kind != MO_SLOT) continue;

                if (op->slot >= f->nslots) {
                    printf("frame_layout: slot %d in '%s', which only has %d\n",
                           op->slot, f->name, f->nslots);
                    exit(1);
                }

                *op = m_mem(op->slot * SLOT_SIZE, MEM_OFF);
            }
        }
    }
}

/* stp  x29, x30, [sp, #-16]!     save the old frame pointer and the return address
 * add  x29, sp, #0               x29 = sp  (mov x29, sp)
 * sub  sp, sp, #frame_size       room for the slots  */
static void add_prologue(MFunc* f) {
    MBlock* entry = f->head;
    MInstr* first = entry->head;

    m_insert_before(entry, first, m_stp(X29, X30, m_mem(-FP_LR_SIZE, MEM_PRE)));
    m_insert_before(entry, first, m_binop(A_ADD, X29, SP, m_imm(0)));

    if (f->frame_size > 0) {
        m_insert_before(entry, first, m_binop(A_SUB, SP, SP, m_imm(f->frame_size)));
    }
}

/* before every ret:
 * add  sp, x29, #0               drop the slots  (mov sp, x29)
 * ldp  x29, x30, [sp], #16       restore x29 / x30, sp back to where the caller had it */
static void add_epilogues(MFunc* f) {
    for (MBlock* b = f->head; b; b = b->next) {
        for (MInstr* instr = b->head; instr; instr = instr->next) {
            if (instr->op != A_RET) continue;

            m_insert_before(b, instr, m_binop(A_ADD, SP, X29, m_imm(0)));
            m_insert_before(b, instr, m_ldp(X29, X30, m_mem(FP_LR_SIZE, MEM_POST)));
        }
    }
}
