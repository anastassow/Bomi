#include "ir.h"
#include "lower.h"

#include <stdlib.h>

static void lower_function(LoweringContext* ctx, const Node* node) {
    if (!node || node->type != NODE_FUNC) {
        printf("Expected NODE_FUNC\n");
        exit(1);
    }

    ctx->current_func = ir_func_new(
        ctx->program,
        node->func.name
    );

    ctx->current_block = ir_block_new(
        ctx->current_func,
        "entry"
    );

    if (node->func.body) {
        for (const Node* stmt = node->func.body->head;
             stmt;
             stmt = stmt->next) {

            lower_statement(ctx, stmt);
             }
    }
}

static void lower_statement(LoweringContext* ctx, const Node* node) {

    Operand result;

    switch (node->type) {
        case NODE_RETURN: {
             result = node->ret.expr
                ? lower_expr(ctx, node->ret.expr)
                : ir_none();

            ir_ret(ctx->current_block, result);
            break;
        }

        // made binop and num for now JUST FOR TESTING
        case NODE_BINOP:
        case NODE_NUM:
            (void)lower_expr(ctx, node);
            break;

        default:
            printf("Unsupported statement type: %d\n", node->type);
            exit(1);
    }
}

static Operand lower_expr(LoweringContext* ctx, const Node* node) {
    if (!node) {
        fprintf(stderr, "NULL expression\n");
        exit(EXIT_FAILURE);
    }

    switch (node->type) {
        case NODE_NUM:
            return ir_imm(node->num.value);

        case NODE_BINOP: {
            Operand left = lower_expr(ctx, node->binop.l);
            Operand right = lower_expr(ctx, node->binop.r);

            IrOp op = lower_operator(node->binop.op);

            return ir_binop(
                ctx->current_func,
                ctx->current_block,
                op,
                left,
                right
            );
        }

        default:
            printf("Unsupported expression type: %d\n", node->type);
            exit(1);
    }
}

static IrOp lower_operator(Op op) {
    switch (op) {
        case OP_ADD:  return IR_ADD;
        case OP_SUB:  return IR_SUB;
        case OP_MUL:  return IR_MUL;
        case OP_DIV:  return IR_SDIV;
        case OP_MOD:  return IR_SREM;

        case OP_BAND: return IR_AND;
        case OP_BOR:  return IR_OR;
        case OP_BXOR: return IR_XOR;
        case OP_SHL:  return IR_SHL;
        case OP_SHR:  return IR_SHR;

        default:
            printf("Unsupported operator: %d\n", op);
            exit(1);
    }
}

static void lower_global(LoweringContext* ctx, const Node* node ) {
    if (!node) return;

    switch (node->type) {
        case NODE_FUNC:
            lower_function(ctx, node);
            break;

            // Future AST nodes

        default:
            fprintf(stderr,
                    "Unsupported global node type: %d\n",
                    node->type);
            exit(EXIT_FAILURE);
    }
}

IrProgram* lower_program(const Node* root) {
    if (!root || root->type != NODE_PROGRAM) {
        printf("Expected NODE_PROGRAM\n");
        exit(1);
    }

    LoweringContext ctx = {
        .program = ir_program_new(),
        .current_func = NULL,
        .current_block = NULL
    };

    if (root->program.program) {
        for (const Node* node = root->program.program->head;
             node;
             node = node->next) {

             lower_global(&ctx, node);
             }
    }

    return ctx.program;
}