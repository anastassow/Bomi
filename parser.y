%code requires {
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "lower.h"
#include "codegen.h"
}

%code {
#include <stdio.h>
int yylex(void);
void yyerror(const char *s);

IrProgram* p;
Node* root;
}

%destructor { free_tree($$); } <n>

%union {
	struct Node* n;
	char* s;
	long ival;
	LinkedList* list;
}

%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN TYPE_NAME

%token <s> IDENTIFIER
%token STRING_LITERAL SIZEOF

%token <ival> CONSTANT

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE CONST VOLATILE VOID
%token STRUCT UNION ENUM ELLIPSIS

%type <n> statement expression_statement jump_statement
%type <n> primary_expression postfix_expression unary_expression cast_expression
%type <n> multiplicative_expression additive_expression shift_expression
%type <n> relational_expression equality_expression and_expression
%type <n> exclusive_or_expression inclusive_or_expression logical_and_expression
%type <n> logical_or_expression conditional_expression assignment_expression
%type <n> expression
%type <s> declarator direct_declarator
%type <list> statement_list compound_statement

%token CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%start translation_unit
%%

primary_expression
	: IDENTIFIER { free($1); $$ = NULL; }
	| CONSTANT { $$ = new_num($1); }
	| STRING_LITERAL
	| '(' expression ')' { $$ = $2; }
	;

postfix_expression
	: primary_expression
	| postfix_expression '[' expression ']'
	| postfix_expression '(' ')'
	| postfix_expression '(' argument_expression_list ')'
	| postfix_expression '.' IDENTIFIER
	| postfix_expression PTR_OP IDENTIFIER
	| postfix_expression INC_OP
	| postfix_expression DEC_OP
	;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression
	;

unary_expression
	: postfix_expression
	| INC_OP unary_expression
	| DEC_OP unary_expression
	| unary_operator cast_expression
	| SIZEOF unary_expression
	| SIZEOF '(' type_name ')'
	;

unary_operator
	: '&'
	| '*'
	| '+'
	| '-'
	| '~'
	| '!'
	;

cast_expression
	: unary_expression
	| '(' type_name ')' cast_expression
	;

multiplicative_expression
	: cast_expression
	| multiplicative_expression '*' cast_expression { $$ = new_binop(OP_MUL, $1, $3); }
	| multiplicative_expression '/' cast_expression	{ $$ = new_binop(OP_DIV, $1, $3); }
	| multiplicative_expression '%' cast_expression { $$ = new_binop(OP_MOD, $1, $3); }
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression { $$ = new_binop(OP_ADD, $1, $3); }
	| additive_expression '-' multiplicative_expression { $$ = new_binop(OP_SUB, $1, $3); }
	;

shift_expression
	: additive_expression
	| shift_expression LEFT_OP additive_expression
	| shift_expression RIGHT_OP additive_expression
	;

relational_expression
	: shift_expression
	| relational_expression '<' shift_expression
	| relational_expression '>' shift_expression
	| relational_expression LE_OP shift_expression
	| relational_expression GE_OP shift_expression
	;

equality_expression
	: relational_expression
	| equality_expression EQ_OP relational_expression
	| equality_expression NE_OP relational_expression
	;

and_expression
	: equality_expression
	| and_expression '&' equality_expression { $$ = new_binop(OP_BAND, $1, $3); }
	;

exclusive_or_expression
	: and_expression
	| exclusive_or_expression '^' and_expression { $$ = new_binop(OP_BXOR, $1, $3); }
	;

inclusive_or_expression
	: exclusive_or_expression
	| inclusive_or_expression '|' exclusive_or_expression { $$ = new_binop(OP_BOR, $1, $3); }
	;

logical_and_expression
	: inclusive_or_expression
	| logical_and_expression AND_OP inclusive_or_expression
	;

logical_or_expression
	: logical_and_expression
	| logical_or_expression OR_OP logical_and_expression
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression '?' expression ':' conditional_expression
	;

assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression
	;

assignment_operator
	: '='
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

expression
	: assignment_expression
	| expression ',' assignment_expression
	;

constant_expression
	: conditional_expression
	;

declaration
	: declaration_specifiers ';'
	| declaration_specifiers init_declarator_list ';'
	;

declaration_specifiers
	: storage_class_specifier
	| storage_class_specifier declaration_specifiers
	| type_specifier
	| type_specifier declaration_specifiers
	| type_qualifier
	| type_qualifier declaration_specifiers
	;

init_declarator_list
	: init_declarator
	| init_declarator_list ',' init_declarator
	;

init_declarator
	: declarator
	| declarator '=' initializer
	;

storage_class_specifier
	: TYPEDEF
	| EXTERN
	| STATIC
	| AUTO
	| REGISTER
	;

type_specifier
	: VOID
	| CHAR
	| SHORT
	| INT
	| LONG
	| FLOAT
	| DOUBLE
	| SIGNED
	| UNSIGNED
	| struct_or_union_specifier
	| enum_specifier
	| TYPE_NAME
	;

struct_or_union_specifier
	: struct_or_union IDENTIFIER '{' struct_declaration_list '}'
	| struct_or_union '{' struct_declaration_list '}'
	| struct_or_union IDENTIFIER
	;

struct_or_union
	: STRUCT
	| UNION
	;

struct_declaration_list
	: struct_declaration
	| struct_declaration_list struct_declaration
	;

struct_declaration
	: specifier_qualifier_list struct_declarator_list ';'
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list
	| type_specifier
	| type_qualifier specifier_qualifier_list
	| type_qualifier
	;

struct_declarator_list
	: struct_declarator
	| struct_declarator_list ',' struct_declarator
	;

struct_declarator
	: declarator
	| ':' constant_expression
	| declarator ':' constant_expression
	;

enum_specifier
	: ENUM '{' enumerator_list '}'
	| ENUM IDENTIFIER '{' enumerator_list '}'
	| ENUM IDENTIFIER
	;

enumerator_list
	: enumerator
	| enumerator_list ',' enumerator
	;

enumerator
	: IDENTIFIER
	| IDENTIFIER '=' constant_expression
	;

type_qualifier
	: CONST
	| VOLATILE
	;

declarator
	: pointer direct_declarator	{ $$ = $2; }
	| direct_declarator		{ $$ = $1; }
	;

direct_declarator
	: IDENTIFIER					{ $$ = $1; }
	| '(' declarator ')'				{ $$ = $2; }
	| direct_declarator '[' constant_expression ']'	{ $$ = $1; }
	| direct_declarator '[' ']'			{ $$ = $1; }
	| direct_declarator '(' parameter_type_list ')'	{ $$ = $1; }
	| direct_declarator '(' identifier_list ')'	{ $$ = $1; }
	| direct_declarator '(' ')' 			{ $$ = $1; }
	;

pointer
	: '*'
	| '*' type_qualifier_list
	| '*' pointer
	| '*' type_qualifier_list pointer
	;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;


parameter_type_list
	: parameter_list
	| parameter_list ',' ELLIPSIS
	;

parameter_list
	: parameter_declaration
	| parameter_list ',' parameter_declaration
	;

parameter_declaration
	: declaration_specifiers declarator
	| declaration_specifiers abstract_declarator
	| declaration_specifiers
	;

identifier_list
	: IDENTIFIER
	| identifier_list ',' IDENTIFIER
	;

type_name
	: specifier_qualifier_list
	| specifier_qualifier_list abstract_declarator
	;

abstract_declarator
	: pointer
	| direct_abstract_declarator
	| pointer direct_abstract_declarator
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
	| '[' ']'
	| '[' constant_expression ']'
	| direct_abstract_declarator '[' ']'
	| direct_abstract_declarator '[' constant_expression ']'
	| '(' ')'
	| '(' parameter_type_list ')'
	| direct_abstract_declarator '(' ')'
	| direct_abstract_declarator '(' parameter_type_list ')'
	;

initializer
	: assignment_expression
	| '{' initializer_list '}'
	| '{' initializer_list ',' '}'
	;

initializer_list
	: initializer
	| initializer_list ',' initializer
	;

statement
	: labeled_statement 	{ $$ = NULL; }
	| compound_statement
	| expression_statement
	| selection_statement	{ $$ = NULL; }
	| iteration_statement	{ $$ = NULL; }
	| jump_statement
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'					{ $$ = list_new(); }
	| '{' statement_list '}'			{ $$ = $2; }
	| '{' declaration_list '}'			{ $$ = list_new(); }
	| '{' declaration_list statement_list '}'	{ $$ = $3; }
	;

declaration_list
	: declaration
	| declaration_list declaration
	;

statement_list
	: statement			{ $$ = list_new(); list_push($$, $1); }
	| statement_list statement	{ $$ = $1; list_push($$, $2); }
	;

expression_statement
	: ';'			{ $$ = NULL; }
	| expression ';'	{ $$ = $1; }
	;

selection_statement
	: IF '(' expression ')' statement
	| IF '(' expression ')' statement ELSE statement
	| SWITCH '(' expression ')' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement
	| DO statement WHILE '(' expression ')' ';'
	| FOR '(' expression_statement expression_statement ')' statement
	| FOR '(' expression_statement expression_statement expression ')' statement
	;

jump_statement
	: GOTO IDENTIFIER ';'
	| CONTINUE ';'
	| BREAK ';'
	| RETURN ';'			{ $$ = new_return(NULL); }
	| RETURN expression ';'		{ $$ = new_return($2); }
	;

translation_unit
	: external_declaration
	| translation_unit external_declaration
	;

external_declaration
	: function_definition
	| declaration
	;

function_definition
	: declaration_specifiers declarator declaration_list compound_statement
	| declaration_specifiers declarator compound_statement	 { list_push(root->program.program, new_func($2, $3)); }
	| declarator declaration_list compound_statement
	| declarator compound_statement
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
	root = new_program(list_new());
        int rc = yyparse();
        // if (rc == 0) ir_print(p, stdout);

	// codegen_x86(p, stdout);

	print_tree(root, stdout);
        free_tree(root);

        ir_program_free(p);

    return rc;
}