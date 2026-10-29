#include  "ir/ir.h"
#include  <stdlib.h>
#include <string.h>

#define ALLOCATE_MEMORY_FOR_STRINGS(length, str) \
    do { \
        (str) = calloc( (length) + 1, sizeof(char)); \
        if (!(str)) { printf("Calloc failed at line %d in file: %s", __LINE__, __FILE__); exit(1); } \
    } while (0)

IrProgram* ir_program_new(void) {
    IrProgram* p = calloc(1, sizeof(IrProgram));
    if (!p) {
        printf("Calloc failed at line %d in file: %s", __LINE__, __FILE__);
        exit(1);
    }

    return p;
}

IrFunc* ir_func_new(IrProgram* p, const char* name) {
    IrFunc* f = calloc(1, sizeof(IrFunc));
    if (!f) {
        printf("Calloc failed at line %d in file: %s", __LINE__, __FILE__);
        exit(1);
    }

    size_t len = strlen(name);
    ALLOCATE_MEMORY_FOR_STRINGS(len, f->name);
    strcpy(f->name, name);

    if (!p->head) {
        p->head = f;
    } else {
        p->tail->next = f;
    }

    p->tail = f;

    return f;
}

IrBlock* ir_block_new(IrFunc* f, const char* label) {
    IrBlock* b = calloc(1, sizeof(IrBlock));
    if (!b) {
        printf("Calloc failed at lined %d in file %s", __LINE__, __FILE__);
        exit(1);
    }

    size_t len = strlen(label);
    ALLOCATE_MEMORY_FOR_STRINGS(len, b->label);
    strcpy(b->label, label);

    if (!f->head) {
        f->head = b;
    } else {
        f->tail->next = b;
    }

    f->tail = b;

    return b;
}

Operand ir_imm(long value) {
    Operand op = {
        .kind = OPD_IMM,
        .imm = value
    };

    return op;
}

Operand ir_none(void) {
    Operand op = {
        .kind = OPD_NONE,
    };

    return op;
}

Operand ir_vreg(int vreg_next) {
    Operand op = {
        .kind = OPD_VREG,
        .vreg = vreg_next
    };

    return op;
}