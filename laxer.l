D			[0-9]

%{
#include <stdlib.h>
#include "calc.tab.h"
%}
%option noyywrap

%%
{D}+			{ yylval = strtol(yytext, NULL, 10); return(CONSTANT); }

";"			{ return(';'); }
"("			{ return('('); }
")"			{ return(')'); }
"+"			{ return('+'); }
"-"			{ return('-'); }
"*"			{ return('*'); }
"/"			{ return('/'); }

[ \t\v\n\f]		{ }
.			{ return(yytext[0]); }

%%