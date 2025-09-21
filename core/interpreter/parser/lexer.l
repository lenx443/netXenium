option noyywrap
%option nounistd
%option header-file="lexer.h"
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"  // tokens de Bison

// yylval debe ser declarado extern en Bison
extern YYSTYPE yylval;
%}

%x STRING SINGLE_STRING

%%

[ \t]+          ;  // ignorar espacios y tabs
"#".*           ;  // ignorar comentarios
\n|;            { return TKN_NEWLINE; }
"("             { return TKN_LPARENT; }
")"             { return TKN_RPARENT; }
"{"             { return TKN_LBRACE; }
"}"             { return TKN_RBRACE; }
"=>"             { return TKN_BLOCK; }
"="             { return TKN_ASSIGNMENT; }

\"              { BEGIN(STRING); yylval.str = strdup(""); }
\'              { BEGIN(SINGLE_STRING); yylval.str = strdup(""); }

\$[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext + 1);
    return TKN_PROPERTY;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext);
    return TKN_IDENTIFIER;
}

<STRING>[^\\\"\n]+ {
    char *tmp = malloc(strlen(yylval.str) + yyleng + 1);
    strcpy(tmp, yylval.str);
    strncat(tmp, yytext, yyleng);
    free(yylval.str);
    yylval.str = tmp;
}

<STRING>\\[nrtvfa\\\'\"?] {
    char c = yytext[1];
    char *tmp = malloc(strlen(yylval.str)+2);
    strcpy(tmp, yylval.str);
    size_t len = strlen(tmp);
    tmp[len] = (c=='n'?'\n': c=='r'?'\r': c=='t'?'\t':
                c=='v'?'\v': c=='f'?'\f': c=='a'?'\a':
                c=='\\'? '\\': c=='\'? '\'': c=='\"'? '\"':'?');
    tmp[len+1] = '\0';
    free(yylval.str);
    yylval.str = tmp;
}

<STRING>\" { BEGIN(INITIAL); return TKN_STRING; }

<SINGLE_STRING>[^\\'\n]+ {
    char *tmp = malloc(strlen(yylval.str) + yyleng + 1);
    strcpy(tmp, yylval.str);
    strncat(tmp, yytext, yyleng);
    free(yylval.str);
    yylval.str = tmp;
}

<SINGLE_STRING>\\[nrtvfa\\\'\"?] {
    char c = yytext[1];
    char *tmp = malloc(strlen(yylval.str)+2);
    strcpy(tmp, yylval.str);
    size_t len = strlen(tmp);
    tmp[len] = (c=='n'?'\n': c=='r'?'\r': c=='t'?'\t':
                c=='v'?'\v': c=='f'?'\f': c=='a'?'\a':
                c=='\\'? '\\': c=='\'? '\'': c=='\"'? '\"':'?');
    tmp[len+1] = '\0';
    free(yylval.str);
    yylval.str = tmp;
}

<SINGLE_STRING>\' { BEGIN(INITIAL); return TKN_STRING; }

. { return TKN_UNDEFINED; }

%%

int yywrap() { return 1; }
