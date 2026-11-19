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

IrFunc* ir_func_new(IrProgram* p, const char* name, IrType ret_type) {
    IrFunc* f = calloc(1, sizeof(IrFunc));
    if (!f) {
        printf("Calloc failed at line %d in file: %s", __LINE__, __FILE__);
        exit(1);
    }

    size_t len = strlen(name);
    ALLOCATE_MEMORY_FOR_STRINGS(len, f->name);
    strcpy(f->name, name);
    f->ret_type = ret_type;

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

int ir_block_terminated(const IrBlock* b) {
    return b && b->tail && b->tail->op == IR_RET;
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

/* Allocates an instruction with its tag, type and dst set, and appends it to b.
 * The caller fills in the union member that matches op. */
static IrInstr* ir_instr_append(IrBlock* b, IrOp op, IrType type, int dst) {
    IrInstr* instr = calloc(1, sizeof(IrInstr));
    if (!instr) {
        printf("Calloc failed on line: %d in file: %s", __LINE__, __FILE__);
        exit(1);
    }

    instr->op = op;
    instr->type = type;
    instr->dst = dst;

    if (!b->head) {
        b->head = instr;
    } else {
        b->tail->next = instr;
    }

    b->tail = instr;

    return instr;
}

Operand ir_binop(IrFunc* f, IrBlock* b, IrOp op, Operand l, Operand r) {
    if (!f || !b) {
        printf("Invalid function or block\n");
        exit(1);
    }

    if (op < IR_ADD || op > IR_SHR) {
        printf("ir_binop: op %d is not a binary operation\n", op);
        exit(1);
    }

    IrInstr* instr = ir_instr_append(b, op, IR_I32, f->next_vreg++);
    instr->binop.lhs = l;
    instr->binop.rhs = r;

    return ir_vreg(instr->dst);
}

void ir_ret(IrBlock* b, Operand v) {
    if (!b) {
        printf("Invalid block\n");
        exit(1);
    }

    IrType type = v.kind == OPD_NONE ? IR_VOID : IR_I32;

    IrInstr* instr = ir_instr_append(b, IR_RET, type, -1);
    instr->ret.value = v;
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
        case IR_SHR:  return "ashr";   /* signed int: arithmetic shift */
        case IR_RET:  return "ret";
        default:      return "unknown";
    }
}

static const char* ir_type_to_string(IrType t) {
    switch (t) {
        case IR_VOID: return "void";
        case IR_I32:  return "i32";
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
        const char* ret_ty = ir_type_to_string(f->ret_type);

        fprintf(out, "define %s @%s() {\n", ret_ty, f->name);

        for (const IrBlock* b = f->head; b; b = b->next) {
            fprintf(out, "%s:\n", b->label);

            for (const IrInstr* instr = b->head;
                 instr;
                 instr = instr->next) {

                const char* ty = ir_type_to_string(instr->type);

                switch (instr->op) {
                    case IR_ADD: case IR_SUB: case IR_MUL: case IR_SDIV: case IR_SREM:
                    case IR_AND: case IR_OR:  case IR_XOR: case IR_SHL:  case IR_SHR:
                        fprintf(out, "  %%v%d = %s %s ",
                                instr->dst,
                                ir_op_to_string(instr->op),
                                ty);
                        ir_print_operand(instr->binop.lhs, out);
                        fprintf(out, ", ");
                        ir_print_operand(instr->binop.rhs, out);
                        break;

                    case IR_RET:
                        if (instr->ret.value.kind == OPD_NONE) {
                            fprintf(out, "  ret void");
                        } else {
                            fprintf(out, "  ret %s ", ty);
                            ir_print_operand(instr->ret.value, out);
                        }
                        break;
                }

                fprintf(out, "\n");
            }
        }

        fprintf(out, "}\n\n");
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