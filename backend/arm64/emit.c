//
// Created by Dimitar Anastassov on 21.11.26.
//

#include "emit.h"

#include <stdlib.h>

static void m_print_func(const MFunc* f, FILE* out);


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
    }
}
