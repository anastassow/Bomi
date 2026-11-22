//
// Created by Dimitar Anastassov on 21.11.26.
//

#include "emit.h"

#include <stdlib.h>

static const char* const AOP_NAMES[] = {
    [A_MOV]  = "mov",  [A_MOVZ] = "movz", [A_MOVK] = "movk",
    [A_ADD]  = "add",  [A_SUB]  = "sub",  [A_MUL]  = "mul",  [A_SDIV] = "sdiv", [A_MSUB] = "msub",
    [A_AND]  = "and",  [A_ORR]  = "orr",  [A_EOR]  = "eor",  [A_LSL]  = "lsl",  [A_ASR]  = "asr",
    [A_LDR]  = "ldr",  [A_STR]  = "str",  [A_STP]  = "stp",  [A_LDP]  = "ldp",
    [A_RET]  = "ret",
};

static void m_print_func(const MFunc* f, FILE* out);
static void m_print_instr(const MInstr* instr, FILE* out);
static void print_operand(const MOperand* op, FILE* out);


void m_print(const MProgram* m, FILE* out) {
    if (!m || !out) {
        printf("m_print: invalid program or output\n");
        exit(1);
    }

    fprintf(out, "    .section __TEXT,__text,regular,pure_instructions\n");

    for (const MFunc* f = m->head; f; f = f->next) {
        m_print_func(f, out);
    }
}

/* macOS puts an underscore in front of C names: main -> _main */
static void m_print_func(const MFunc* f, FILE* out) {
    fprintf(out, "\n");
    fprintf(out, "    .globl _%s\n", f->name);
    fprintf(out, "    .p2align 2\n");
    fprintf(out, "_%s:\n", f->name);

    for (const MBlock* b = f->head; b; b = b->next) {
        /* L... labels stay local to the file; the function name keeps them unique */
        fprintf(out, "L%s_%s:\n", f->name, b->label);

        for (const MInstr* instr = b->head; instr; instr = instr->next) {
            m_print_instr(instr, out);
        }
    }
}

static void m_print_instr(const MInstr* instr, FILE* out) {
    if (instr->op < A_MOV || instr->op > A_RET) {
        printf("m_print: unknown instruction %d\n", instr->op);
        exit(1);
    }

    if (instr->nops == 0) {
        fprintf(out, "    %s\n", AOP_NAMES[instr->op]);
        return;
    }

    fprintf(out, "    %-4s ", AOP_NAMES[instr->op]);

    for (int k = 0; k < instr->nops; k++) {
        if (k > 0) fprintf(out, ", ");
        print_operand(&instr->ops[k], out);
    }

    /* movk always fills the high half */
    if (instr->op == A_MOVK) fprintf(out, ", lsl #16");

    fprintf(out, "\n");
}

static void print_operand(const MOperand* op, FILE* out) {
    switch (op->kind) {
        case MO_REG:
            if (op->reg.num == REG_SP) {
                fprintf(out, "sp");
            } else {
                fprintf(out, "%c%d", op->reg.is64 ? 'x' : 'w', op->reg.num);
            }
            break;

        case MO_IMM:
            fprintf(out, "#%ld", op->imm);
            break;

        case MO_MEM:
            switch (op->mem.mode) {
                case MEM_OFF:  fprintf(out, "[sp, #%d]", op->mem.off);  break;
                case MEM_PRE:  fprintf(out, "[sp, #%d]!", op->mem.off); break;
                case MEM_POST: fprintf(out, "[sp], #%d", op->mem.off);  break;
            }
            break;

        case MO_SLOT:
            printf("m_print: [slot %d] is still there, frame_layout must run before m_print\n", op->slot);
            exit(1);
    }
}
