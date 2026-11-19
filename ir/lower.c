#include "ir.h"
#include "lower.h"

#include <stdlib.h>
#include <string.h>

/* top level */
static void    lower_global(LoweringContext* ctx, const Node* node);
static void    lower_function(LoweringContext* ctx, const Node* node);

/* statements */
static void    lower_statement(LoweringContext* ctx, const Node* node);
static void    lower_declaration(LoweringContext* ctx, const Node* node);
static void    lower_return(LoweringContext* ctx, const Node* node);
static void    lower_expr_stmt(LoweringContext* ctx, const Node* node);

/* expressions */
static Operand lower_expr(LoweringContext* ctx, const Node* node);
static Operand lower_num(LoweringContext* ctx, const Node* node);
static Operand lower_var(LoweringContext* ctx, const Node* node);
static Operand lower_binop(LoweringContext* ctx, const Node* node);

/* helpers */
static IrType  lower_type(TypeKind t);
static IrOp    lower_operator(Op op);

/* symbol table (locals of the current function) */
static Symbol* symbol_lookup(const LoweringContext* ctx, const char* name);
static void    symbol_add(LoweringContext* ctx, const char* name, Operand addr);
static void    symbols_free(LoweringContext* ctx);


/* top level implementation  */

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

static void lower_function(LoweringContext* ctx, const Node* node) {
    if (!node || node->type != NODE_FUNC) {
        printf("Expected NODE_FUNC\n");
        exit(1);
    }

    IrType ret_type = lower_type(node->func.return_type);

    ctx->current_func = ir_func_new(
        ctx->program,
        node->func.name,
        ret_type
    );

    ctx->current_block = ir_block_new(
        ctx->current_func,
        "entry"
    );

    if (node->func.body) {
        for (const Node* stmt = node->func.body->head;
            stmt;
            stmt = stmt->next) {

            if (ir_block_terminated(ctx->current_block)) break;

            lower_statement(ctx, stmt);
            }
    }

    if (!ir_block_terminated(ctx->current_block)) {
        ir_ret(ctx->current_block,
               ret_type == IR_VOID ? ir_none() : ir_imm(0));
    }

    symbols_free(ctx);
}

/* statements implementation  */

static void lower_statement(LoweringContext* ctx, const Node* node) {

    switch (node->type) {

        case NODE_DECLARATION: {
            lower_declaration(ctx, node);
            break;
        }

        case NODE_RETURN: {
            lower_return(ctx, node);
            break;
        }

        case NODE_EXPR_STMT: {
            lower_expr_stmt(ctx, node);
            break;
        }

        default:
            printf("Unsupported statement type: %d\n", node->type);
            exit(1);
    }
}

/* int x = init;  ->  %vN = alloca i32
 *                    store i32 <init>, ptr %vN      (only if there is an init) */
static void lower_declaration(LoweringContext* ctx, const Node* node) {
    if (!node || node->type != NODE_DECLARATION) {
        printf("Expected NODE_DECLARATION\n");
        exit(1);
    }

    if (symbol_lookup(ctx, node->decl.name)) {
        printf("Redeclaration of '%s'\n", node->decl.name);
        exit(1);
    }

    Operand slot = ir_alloca(ctx->current_func, ctx->current_block);

    symbol_add(ctx, node->decl.name, slot);

    if (node->decl.init) {
        Operand value = lower_expr(ctx, node->decl.init);
        ir_store(ctx->current_block, value, slot);
    }
}

/* return expr;  ->  ret i32 <expr>
 * return;       ->  ret void
 * for now only for int */
static void lower_return(LoweringContext* ctx, const Node* node) {
    if (!node || node->type != NODE_RETURN) {
        printf("Expected NODE_RETURN\n");
        exit(1);
    }

    IrFunc* f = ctx->current_func;

    if (!node->ret.expr) {
        if (f->ret_type != IR_VOID) {
            printf("Function '%s' must return a value\n", f->name);
            exit(1);
        }

        ir_ret(ctx->current_block, ir_none());
        return;
    }

    if (f->ret_type == IR_VOID) {
        printf("Void function '%s' cannot return a value\n", f->name);
        exit(1);
    }

    Operand value = lower_expr(ctx, node->ret.expr);
    ir_ret(ctx->current_block, value);
}

/* expr;  ->  lower expr for its side effects (e.g. x = x * 3;), drop the result */
static void lower_expr_stmt(LoweringContext* ctx, const Node* node) {
    if (!ctx || !ctx->current_func || !ctx->current_block) {
        printf("Expression statement outside of a function\n");
        exit(1);
    }

    if (!node || node->type != NODE_EXPR_STMT) {
        printf("Expected NODE_EXPR_STMT\n");
        exit(1);
    }

    if (!node->expr_stmt.expr) return;

    (void)lower_expr(ctx, node->expr_stmt.expr);
}

/* expressions implementation */

static Operand lower_expr(LoweringContext* ctx, const Node* node) {
    if (!ctx || !ctx->current_func || !ctx->current_block) {
        fprintf(stderr, "Expression outside of a function\n");
        exit(EXIT_FAILURE);
    }

    if (!node) {
        fprintf(stderr, "NULL expression\n");
        exit(EXIT_FAILURE);
    }

    if (node->type == NODE_EXPR_STMT) {
        node = node->expr_stmt.expr;

        if (!node) return ir_none();
    }

    switch (node->type) {
        case NODE_NUM:      return lower_num(ctx, node);
        case NODE_BINOP:    return lower_binop(ctx, node);
        case NODE_VAR:      return lower_var(ctx, node);

        default:
            printf("Unsupported expression type: %d\n", node->type);
            exit(1);
    }
}

/* 42  ->  the constant itself, no instruction needed */
static Operand lower_num(LoweringContext* ctx, const Node* node) {
    (void)ctx;

    if (!node || node->type != NODE_NUM) {
        printf("Expected NODE_NUM\n");
        exit(1);
    }

    return ir_imm(node->num.value);
}

/* x  ->  %vN = load i32, ptr <slot of x> */
static Operand lower_var(LoweringContext* ctx, const Node* node) {
    if (!node || node->type != NODE_VAR) {
        printf("Expected NODE_VAR\n");
        exit(1);
    }

    const Symbol* sym = symbol_lookup(ctx, node->var.name);
    if (!sym) {
        printf("Use of undeclared variable '%s'\n", node->var.name);
        exit(1);
    }

    return ir_load(ctx->current_func, ctx->current_block, sym->addr);
}

/* l op r  ->  lower l, lower r, then %vN = op i32 l, r */
static Operand lower_binop(LoweringContext* ctx, const Node* node) {
    if (!node || node->type != NODE_BINOP) {
        printf("Expected NODE_BINOP\n");
        exit(1);
    }

    if (!node->binop.l || !node->binop.r) {
        printf("Binary operation is missing an operand\n");
        exit(1);
    }

    IrOp op = lower_operator(node->binop.op);

    Operand l = lower_expr(ctx, node->binop.l);
    Operand r = lower_expr(ctx, node->binop.r);

    return ir_binop(ctx->current_func, ctx->current_block, op, l, r);
}

/* helpers implementation */

static IrType lower_type(TypeKind t) {
    switch (t) {
        case TYPE_INT: return IR_I32;
        default:
            fprintf(stderr, "Unsupported type: %d\n", t);
            exit(EXIT_FAILURE);
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

static Symbol* symbol_lookup(const LoweringContext* ctx, const char* name) {
    for (Symbol* s = ctx->locals; s; s = s->next) {
        if (strcmp(s->name, name) == 0) return s;
    }

    return NULL;
}

static void symbol_add(LoweringContext* ctx, const char* name, Operand addr) {
    Symbol* s = calloc(1, sizeof(Symbol));
    if (!s) {
        printf("Calloc failed on line: %d in file: %s", __LINE__, __FILE__);
        exit(1);
    }

    s->name = name;
    s->addr = addr;
    s->next = ctx->locals;
    ctx->locals = s;
}

static void symbols_free(LoweringContext* ctx) {
    Symbol* s = ctx->locals;

    while (s) {
        Symbol* next = s->next;
        free(s);
        s = next;
    }

    ctx->locals = NULL;
}

IrProgram* lower_program(const Node* root) {
    if (!root || root->type != NODE_PROGRAM) {
        printf("Expected NODE_PROGRAM\n");
        exit(1);
    }

    LoweringContext ctx = {
        .program = ir_program_new(),
        .current_func = NULL,
        .current_block = NULL,
        .locals = NULL
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