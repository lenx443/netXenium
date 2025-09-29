#ifndef __PARSER_H__
#define __PARSER_H__

#include "instance.h"
#include "lexer.h"

typedef struct {
  Lexer *lexer;
  Lexer_Token token;
} Parser;

void parser_next(Parser *);
Xen_Instance *parser_stmt(Parser *);

#endif
