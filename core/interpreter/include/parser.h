#ifndef __PARSER_H__
#define __PARSER_H__

#include "ast.h"
#include "lexer.h"

typedef struct {
  Lexer *lexer;
  Lexer_Token token;
} Parser;

void parser_next(Parser *);
int parser_stmt(Parser *, AST_Node_t **);
int parser_string(Parser *, AST_Node_t **);
int parser_literal(Parser *, AST_Node_t **);
int parser_property(Parser *, AST_Node_t **);
int parser_assignment_rhs(const char *, Parser *, AST_Node_t **);
int parser_block(Parser *, AST_Node_t ***, size_t *);
AST_Node_t *parser_arg(Parser *);

#endif
