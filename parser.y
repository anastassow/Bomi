%code requires {
#include "ast.h"
}

%code {
#include <stdio.h>
int yylex(void);
void yyerror(const char *s);

Node* ast_root;
}

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
    : expression ';' { ast_root = $1; print_tree(ast_root, stdout); printf("Answer: %ld", eval(ast_root)); printf("\n--------------------------------------------\n"); free_tree(ast_root); }
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

    if (yyparse() != 0)
        return 1;

    print_tree(ast_root, stdout);
    free_tree(ast_root);
    return 0;
}