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
