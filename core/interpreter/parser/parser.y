%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "logs.h"

void yyerror(const char *s);
extern int yylex(void);
extern int yylineno;

#define error(msg, ...) log_add(NULL, ERROR, "Parser", msg, ##__VA_ARGS__)
%}

/* Valores semánticos */
%union {
    char *str;
    AST_Node_t *node;
    AST_Node_t **block;       /* array de nodos */
    AST_Node_t **stmt_list;   /* lista de statements */
}

/* Tokens del lexer */
%token <str> TKN_IDENTIFIER TKN_STRING TKN_PROPERTY
%token TKN_ASSIGNMENT TKN_NEWLINE TKN_LBRACE TKN_RBRACE TKN_LPARENT TKN_RPARENT TKN_BLOCK

/* Tipos de reglas */
%type <node> stmt assignment cmd arg literal property string_rule
%type <stmt_list> stmt_list
%type <block> block
%type <stmt_list> arg_list

%%

program:
      stmt_list
    ;

stmt_list:
      /* vacío */ { $$ = NULL; }
    | stmt_list stmt TKN_NEWLINE {
          /* agregar stmt a la lista dinámica si quieres */
          $$ = $1;  /* $1 es stmt_list */
      }
    ;

stmt:
      assignment
    | cmd
    ;

assignment:
      TKN_IDENTIFIER TKN_ASSIGNMENT string_rule {
            $$ = ast_make_assignment($1, $3);
            free($1);
      }
    ;

cmd:
      TKN_IDENTIFIER TKN_LPARENT arg_list TKN_RPARENT {
            $$ = ast_make_cmd($1, $3, $3 ? $3[0]->child_count : 0); // Ajustar según tu AST
            free($1);
      }
    ;

arg_list:
      /* vacío */ { $$ = NULL; }
    | arg_list arg {
          /* realloc y agregar arg al array si quieres */
          $$ = $1;
      }
    ;

arg:
      string_rule { $$ = $1; }
    | literal { $$ = $1; }
    | property { $$ = $1; }
    ;

string_rule:
      TKN_STRING { $$ = ast_make_string($1); free($1); }
    ;

literal:
      TKN_IDENTIFIER { $$ = ast_make_literal($1); free($1); }
    ;

property:
      TKN_PROPERTY { $$ = ast_make_property($1); free($1); }
    ;

block:
      TKN_BLOCK TKN_LBRACE stmt_list TKN_RBRACE {
          $$ = $3; /* $3 es stmt_list, que ahora tiene tipo declarado */
      }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}
