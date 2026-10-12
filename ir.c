//
// Created by Dimitar Anastassov on 29.09.26.
//

#include "ir.h"

#include <stdlib.h>

#include <assert.h>
#include <stdio.h>


IrProgram* ir_program_new(void) {
    IrProgram* p = calloc(1, sizeof(IrProgram));
    if ( p == NULL ) { printf("calloc failed in file: %s and line: %d ", __FILE__, __LINE__); exit(1); }
    return p;
}

void ir_program_free(IrProgram *p) {
    if (!p) { return; }
    Instr* i = p->head;
    while (i != NULL) {
        Instr* next = i->next;
        free(i);
        i = next;
    }
    free(p);
}

Operand ir_imm(long value) { return (Operand){.kind = OPD_IMM, .imm = value}; }
Operand ir_vreg(int n) { return (Operand){.kind = OPD_VREG, .vreg = n}; }
Operand ir_none(void) { return (Operand){.kind = OPD_NONE, .imm = 0}; }

static Operand ir_emit(IrProgram *p, IrOp op, Operand a, Operand b) {
    Instr* i = calloc(1, sizeof(Instr));
    if (!i ) { printf(" calloc failed in file: %s and line: %d", __FILE__, __LINE__); exit(1); }

    i->op = op;
    i->a = a;
    i->b = b;
    i->dst = (op == IR_PRINT) ? -1 : p->next_vreg++;

    if (p->tail)    p->tail->next   = i;
    else            p->head         = i;

    p->tail = i;

    return i->dst >= 0 ? ir_vreg(i->dst) : ir_none();
}
Operand ir_binop(IrProgram *p, IrOp op, Operand l, Operand r) {
    assert(op == IR_ADD || op == IR_SUB || op == IR_MUL || op == IR_SDIV);
    assert(l.kind != OPD_NONE && r.kind != OPD_NONE);
    return ir_emit(p, op, l, r);
}
Operand ir_neg(IrProgram *p, Operand v) {
    assert(v.kind != OPD_NONE);
    return ir_emit(p, IR_NEG, v, ir_none());

}
void ir_print_val(IrProgram *p, Operand v) {
    assert(v.kind != OPD_NONE);
    ir_emit(p, IR_PRINT, v, ir_none());
}



// ----------------

static void print_operand(Operand o, FILE *out) {
    switch (o.kind) {
        case OPD_IMM:  fprintf(out, "%ld", o.imm);    break;
        case OPD_VREG: fprintf(out, "%%v%d", o.vreg); break;
        case OPD_NONE: fprintf(out, "<none>");        break;
    }
}

static const char *op_name(IrOp op) {
    switch (op) {
        case IR_ADD:  return "add";
        case IR_SUB:  return "sub";
        case IR_MUL:  return "mul";
        case IR_SDIV: return "sdiv";
        default:      return "?";
    }
}

void ir_print(const IrProgram *p, FILE *out) {
    fprintf(out, "@fmt = private constant [5 x i8] c\"%%ld\\0A\\00\"\n");
    fprintf(out, "declare i32 @printf(ptr, ...)\n\n");
    fprintf(out, "define i32 @main() {\n");
    fprintf(out, "entry:\n");

    int print_count = 0;                          /* names for printf's results: %p0, %p1, ... */

    for (const Instr *i = p->head; i; i = i->next) {
        switch (i->op) {
            case IR_ADD: case IR_SUB: case IR_MUL: case IR_SDIV:
                fprintf(out, "  %%v%d = %s i64 ", i->dst, op_name(i->op));
                print_operand(i->a, out);
                fprintf(out, ", ");
                print_operand(i->b, out);
                fprintf(out, "\n");
                break;

            case IR_NEG:                              /* LLVM has no neg: -x = 0 - x */
                fprintf(out, "  %%v%d = sub i64 0, ", i->dst);
                print_operand(i->a, out);
                fprintf(out, "\n");
                break;

            case IR_PRINT:
                fprintf(out, "  %%p%d = call i32 (ptr, ...) @printf(ptr @fmt, i64 ", print_count++);
                print_operand(i->a, out);
                fprintf(out, ")\n");
                break;
        }
    }

    fprintf(out, "  ret i32 0\n");
    fprintf(out, "}\n");
}