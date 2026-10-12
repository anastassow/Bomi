%code requires { #include "ast.h" }
    
%{
#include <stdio.h>
#include "ir.h"
#include "lower.h"
#include "codegen.h"                     /* ← NEW */

int  yylex(void);
void yyerror(const char *msg);
extern FILE *yyin;
extern int   yylineno;

static IrProgram *prog;      /* the IR for the whole input file */
static int        nerrors;   /* syntax errors seen */
%}

%union {
    long  num;
    Node *node;
}

%token <num> NUM
%token PLUS MINUS TIMES DIVIDE LPAREN RPAREN NEWLINE
%type  <node> expr

%destructor { free_tree($$); } <node>

%left  PLUS MINUS
%left  TIMES DIVIDE
%precedence NEG

%%
input : %empty
      | input line
      ;

line  : NEWLINE
      | expr NEWLINE      {
                            Operand result = lower_expr(prog, $1);   /* AST → IR */
                            ir_print_val(prog, result);              /* add a "print" instruction */
                            free_tree($1);                           /* AST no longer needed */
                          }
      | error NEWLINE     { yyerrok; }
      ;

expr  : NUM                     { $$ = new_num($1); }
      | expr PLUS   expr        { $$ = new_binop('+', $1, $3); }
      | expr MINUS  expr        { $$ = new_binop('-', $1, $3); }
      | expr TIMES  expr        { $$ = new_binop('*', $1, $3); }
      | expr DIVIDE expr        { $$ = new_binop('/', $1, $3); }
      | MINUS expr %prec NEG    { $$ = new_neg($2); }
      | LPAREN expr RPAREN      { $$ = $2; }
      ;
%%

void yyerror(const char *msg) {
    nerrors++;
    fprintf(stderr, "line %d: %s\n", yylineno, msg);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <input file>\n", argv[0]);
        return 1;
    }
    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror(argv[1]);
        return 1;
    }

    prog = ir_program_new();              /* empty IR program               */
    int result = yyparse();               /* parse; each line gets lowered  */
    fclose(yyin);

    if (result == 0 && nerrors == 0)
        codegen_x86(prog, stdout);        /* ← CHANGED: IR → x86-64 assembly */

    ir_program_free(prog);
    return (result || nerrors) ? 1 : 0;
}