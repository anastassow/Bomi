//
// Created by Dimitar Anastassov on 30.09.26.
//

#include "codegen.h"

#include <stdlib.h>

/* Target: x86-64 Linux (ELF, System V ABI), always. */

/* ---------- where does a vreg live? (the ONLY place that decides) ---------- */

/* naive allocation: %vN is at -8*(N+1)(%rbp) */
static int slot(int vreg) { return -8 * (vreg + 1); }

/* emit code that puts operand o into register reg */
static void load(Operand o, const char *reg, FILE *out) {
    switch (o.kind) {
    case OPD_IMM:
        fprintf(out, "    movq    $%ld, %s\n", o.imm, reg);
        break;
    case OPD_VREG:
        fprintf(out, "    movq    %d(%%rbp), %s\n", slot(o.vreg), reg);
        break;
    case OPD_NONE:
        fprintf(stderr, "codegen: missing operand\n");
        abort();
    }
}

/* emit code that stores register reg into vreg's slot */
static void store(const char *reg, int vreg, FILE *out) {
    fprintf(out, "    movq    %s, %d(%%rbp)\n", reg, slot(vreg));
}

/* ---------- prologue / epilogue ---------- */

static void emit_prologue(const IrProgram *p, FILE *out) {
    fprintf(out, "    .section .rodata\n");
    fprintf(out, "fmt:\n    .string \"%%ld\\n\"\n\n");
    fprintf(out, "    .text\n");
    fprintf(out, "    .globl  main\n");
    fprintf(out, "main:\n");
    fprintf(out, "    pushq   %%rbp\n");
    fprintf(out, "    movq    %%rsp, %%rbp\n");

    int frame = (8 * p->next_vreg + 15) & ~15;   /* round up to 16: keeps calls aligned */
    if (frame > 0)
        fprintf(out, "    subq    $%d, %%rsp\n", frame);
}

static void emit_epilogue(FILE *out) {
    fprintf(out, "\n    xorl    %%eax, %%eax\n");   /* return 0 */
    fprintf(out, "    leave\n");                   /* movq %rbp,%rsp ; popq %rbp */
    fprintf(out, "    ret\n");
    fprintf(out, "\n    .section .note.GNU-stack,\"\",@progbits\n");   /* non-executable stack */
}

/* ---------- one template per IR opcode ---------- */

static void emit_instr(const Instr *i, FILE *out) {
    switch (i->op) {
    case IR_ADD:
        fprintf(out, "\n    # %%v%d = add\n", i->dst);
        load(i->a, "%rax", out);
        load(i->b, "%rcx", out);
        fprintf(out, "    addq    %%rcx, %%rax\n");
        store("%rax", i->dst, out);
        break;

    case IR_SUB:
        fprintf(out, "\n    # %%v%d = sub\n", i->dst);
        load(i->a, "%rax", out);
        load(i->b, "%rcx", out);
        fprintf(out, "    subq    %%rcx, %%rax\n");          /* rax = a - b */
        store("%rax", i->dst, out);
        break;

    case IR_MUL:
        fprintf(out, "\n    # %%v%d = mul\n", i->dst);
        load(i->a, "%rax", out);
        load(i->b, "%rcx", out);
        fprintf(out, "    imulq   %%rcx, %%rax\n");
        store("%rax", i->dst, out);
        break;

    case IR_SDIV:
        fprintf(out, "\n    # %%v%d = sdiv\n", i->dst);
        load(i->a, "%rax", out);
        load(i->b, "%rcx", out);
        fprintf(out, "    cqto\n");                          /* sign-extend rax into rdx:rax */
        fprintf(out, "    idivq   %%rcx\n");                 /* rax = rdx:rax / rcx */
        store("%rax", i->dst, out);
        break;

    case IR_NEG:
        fprintf(out, "\n    # %%v%d = neg\n", i->dst);
        load(i->a, "%rax", out);
        fprintf(out, "    negq    %%rax\n");
        store("%rax", i->dst, out);
        break;

    case IR_PRINT:
        fprintf(out, "\n    # print\n");
        load(i->a, "%rsi", out);                           /* 2nd arg: the value  */
        fprintf(out, "    leaq    fmt(%%rip), %%rdi\n");   /* 1st arg: "%%ld\\n"  */
        fprintf(out, "    xorl    %%eax, %%eax\n");        /* varargs: 0 vector regs */
        fprintf(out, "    call    printf\n");
        break;
    }
}

/* ---------- entry point: a linear walk over the instruction list ---------- */

void codegen_x86(const IrProgram *p, FILE *out) {
    emit_prologue(p, out);
    for (const Instr *i = p->head; i; i = i->next)
        emit_instr(i, out);
    emit_epilogue(out);
}


// ----------------------------------------------------------------------
void codegen_arm(const IrProgram *P, FILE *out) {
    
}