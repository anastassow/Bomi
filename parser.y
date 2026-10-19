%code requires {
#include "ast.h"
#include "lower.h"
#include "codegen.h"
}

%code {
#include <stdio.h>
int yylex(void);
void yyerror(const char *s);

IrProgram* p;
}

%destructor { free_tree($$); } <node>

%union {
    long  num;
    Node *node;
}

%token <num> CONSTANT
%type <node> primary_expression multiplicative_expression additive_expression expression

%start translation_unit

%%
primary_expression
    : CONSTANT           { $$ = new_num($1); }
    | '(' expression ')' { $$ = $2; }
    ;

multiplicative_expression
    : primary_expression
    | multiplicative_expression '*' primary_expression { $$ = new_binop('*', $1, $3); }
    | multiplicative_expression '/' primary_expression { $$ = new_binop('/', $1, $3); }
    ;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression { $$ = new_binop('+', $1, $3); }
	| additive_expression '-' multiplicative_expression { $$ = new_binop('-', $1, $3); }
	;

expression
	: additive_expression
	;

expression_statement
    : expression ';' {
	print_tree($1, stdout);
     	printf("Answer: %ld", eval($1));
     	printf("\n--------------------------------------------\n");
     	ir_print_val(p, lower_expr(p, $1));
     	free_tree($1);
     }
    ;

translation_unit
    : expression ';'
    | translation_unit expression_statement
    ;

%%
extern FILE *yyin;

void yyerror(const char *s) { fprintf(stderr, "error: %s\n", s); }

int main(int argc, char **argv) {
    if (argc > 1) {
            yyin = fopen(argv[1], "r");
            if (!yyin) { perror(argv[1]); return 1; }
        }
        p = ir_program_new();
        int rc = yyparse();
        if (rc == 0) ir_print(p, stdout);

	codegen_x86(p, stdout);

        ir_program_free(p);

    return rc;
}