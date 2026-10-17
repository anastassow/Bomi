%{
#include <stdio.h>
int yylex(void);
void yyerror(const char *s);
%}

%define api.value.type {long}

%token CONSTANT

%start translation_unit

%%
primary_expression
    : CONSTANT
    | '(' expression ')' { $$ = $2; }
    ;

multiplicative_expression
    : primary_expression
    | multiplicative_expression '*' primary_expression { $$ = $1 * $3; }
    | multiplicative_expression '/' primary_expression { $$ = $1 / $3; }
    ;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression { $$ = $1 + $3; }
	| additive_expression '-' multiplicative_expression { $$ = $1 - $3; }
	;

expression
	: additive_expression
	;

expression_statement
    : expression ';' { printf("%ld\n", $1); }
    ;

translation_unit
    : expression_statement
    |    translation_unit expression_statement
    ;

%%
extern FILE *yyin;

void yyerror(const char *s) { fprintf(stderr, "error: %s\n", s); }

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) { perror(argv[1]); return 1; }
    }
    return yyparse();
}