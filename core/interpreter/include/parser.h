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
int parser_block(Parser *, AST_Node_t **, size_t *);
int parser_if_cindional(Parser *, AST_Node_t **);
ArgExpr_t *parser_concat(Parser *);
ArgExpr_t *parser_arg(Parser *);

#endif
