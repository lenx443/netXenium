#ifndef __LEXER_H__
#define __LEXER_H__

#include <stddef.h>

#include "macros.h"

typedef enum {
  TKN_EOF = 0,
  TKN_NEWLINE,
  TKN_IDENTIFIER,
  TKN_PROPERTY,
  TKN_STRING,
  TKN_BLOCK,
  TKN_LBRACE,
  TKN_RBRACE,
  TKN_ASSIGNMENT,
  TKN_EQUAL,
  TKN_UNDEFINED,
  TKN_LPARENT,
  TKN_RPARENT
} Lexer_Token_Type;

typedef struct {
  Lexer_Token_Type tkn_type;
  char tkn_text[LXR_TOKEN_TOKEN_SIZE];
} Lexer_Token;

typedef struct {
  const char *src;
  size_t pos;
} Lexer;

void skip_whitespace(Lexer *);
Lexer_Token lexer_next_token(Lexer *);

#endif
