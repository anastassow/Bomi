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

Operand ir_binop(IrFunc* f, IrBlock* b, IrOp op, Operand l, Operand r) {
    if (!f || !b) {
        printf("Invalid function or block\n");
        exit(1);
    }

    IrInstr* instr = calloc(1, sizeof(IrInstr));
    if (!instr) {
        printf("Calloc failed on line: %d in file: %s", __LINE__, __FILE__);
        exit(1);
    }

    instr->op = op;
    instr->dst = f->next_vreg++;
    instr->a = l;
    instr->b = r;

    if (!b->head) {
        b->head = instr;
    } else {
        b->tail->next = instr;
    }

    b->tail = instr;

    return ir_vreg(instr->dst);
}

void ir_ret(IrBlock* b, Operand v) {
    if (!b) {
        printf("Invalid block\n");
        exit(1);
    }

    IrInstr* instr = calloc(1, sizeof(IrInstr));
    if (!instr) {
        printf("Calloc failed on lineL %d and file: %s", __LINE__, __FILE__);
        exit(1);
    }

    instr->op = IR_RET;
    instr->dst = -1;
    instr->a = v;
    instr->b = ir_none();

    if (!b->head) {
        b->head = instr;
    } else {
        b->tail->next = instr;
    }

    b->tail = instr;
}

static const char* ir_op_to_string(IrOp op) {
    switch (op) {
        case IR_ADD:  return "add";
        case IR_SUB:  return "sub";
        case IR_MUL:  return "mul";
        case IR_SDIV: return "sdiv";
        case IR_SREM: return "srem";
        case IR_AND:  return "and";
        case IR_OR:   return "or";
        case IR_XOR:  return "xor";
        case IR_SHL:  return "shl";
        case IR_SHR:  return "shr";
        case IR_RET:  return "ret";
        default:      return "unknown";
    }
}

static void ir_print_operand(Operand op, FILE* out) {
    switch (op.kind) {
        case OPD_IMM:
            fprintf(out, "%ld", op.imm);
            break;

        case OPD_VREG:
            fprintf(out, "%%v%d", op.vreg);
            break;

        case OPD_NONE:
            break;
    }
}

void ir_print(const IrProgram* p, FILE* out) {
    if (!p || !out) return;

    for (const IrFunc* f = p->head; f; f = f->next) {
        fprintf(out, "func %s:\n", f->name);

        for (const IrBlock* b = f->head; b; b = b->next) {
            fprintf(out, "%s:\n", b->label);

            for (const IrInstr* instr = b->head;
                 instr;
                 instr = instr->next) {

                if (instr->op == IR_RET) {
                    fprintf(out, "    ret");

                    if (instr->a.kind != OPD_NONE) {
                        fprintf(out, " ");
                        ir_print_operand(instr->a, out);
                    }
                } else {
                    fprintf(out, "    %%v%d = %s ",
                            instr->dst,
                            ir_op_to_string(instr->op));

                    ir_print_operand(instr->a, out);
                    fprintf(out, ", ");
                    ir_print_operand(instr->b, out);
                }

                fprintf(out, "\n");
                 }
        }

        fprintf(out, "\n");
    }
}

void ir_program_free(IrProgram* p) {
    if (!p) return;

    IrFunc* f = p->head;

    while (f) {
        IrFunc* next_f = f->next;

        IrBlock* b = f->head;

        while (b) {
            IrBlock* next_b = b->next;

            IrInstr* instr = b->head;

            while (instr) {
                IrInstr* next_instr = instr->next;

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