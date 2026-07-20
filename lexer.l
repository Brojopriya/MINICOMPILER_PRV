%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

int yyline = 1;

%}

%%

"int"           { return INT; }
"bool"          { return BOOL; }
"if"            { return IF; }
"else"          { return ELSE; }
"while"         { return WHILE; }
"print"         { return PRINT; }
"true"          { yylval.ival = 1; return BOOL_CONST; }
"false"         { yylval.ival = 0; return BOOL_CONST; }

[0-9]+          { yylval.ival = atoi(yytext); return INT_CONST; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.sval = strdup(yytext); return ID; }

"+"             { return PLUS; }
"-"             { return MINUS; }
"*"             { return MUL; }
"/"             { return DIV; }
"<"             { return LT; }
">"             { return GT; }
"=="            { return EQ; }
"!="            { return NE; }
"&&"            { return AND; }
"||"            { return OR; }
"!"             { return NOT; }
"="             { return ASSIGN; }

";"             { return SEMICOLON; }
"{"             { return LBRACE; }
"}"             { return RBRACE; }
"("             { return LPAREN; }
")"             { return RPAREN; }

"//".*          { }
[ \t]           { }
"\n"            { yyline++; }
.               { fprintf(stderr, "Lexical error: unexpected character '%c' at line %d\n", yytext[0], yyline); }

%%

int yywrap() {
    return 1;
}
