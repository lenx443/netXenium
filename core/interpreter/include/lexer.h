#ifndef __LEXER_H__
#define __LEXER_H__

#include <stddef.h>

#include "macros.h"

typedef enum {
  TKN_EOF = 0,
  TKN_COMMAND,
  TKN_ARGUMENT,
  TKN_PROPERTY,
  TKN_STRING,
  TKN_CONCAT,
  TKN_EQUAL,
  TKN_UNDEFINED,
} Lexer_Token_Type;

typedef struct {
  Lexer_Token_Type tkn_type;
  char tkn_text[LXR_TOKEN_TOKEN_SIZE];
} Lexer_Token;

typedef struct {
  const char *src;
  size_t pos;
  size_t tkn_pos;
} Lexer;

void skip_whitespace(Lexer *);
Lexer_Token lexer_next_token(Lexer *);

#endif
