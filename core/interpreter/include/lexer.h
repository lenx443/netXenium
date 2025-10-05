#ifndef __LEXER_H__
#define __LEXER_H__

#include <stddef.h>

#include "macros.h"

typedef enum {
  TKN_EOF = 0,
  TKN_UNDEFINED,
  TKN_NEWLINE,
  TKN_IDENTIFIER,
  TKN_KEYWORD,
  TKN_NOT,
  TKN_AND,
  TKN_OR,
  TKN_PROPERTY,
  TKN_STRING,
  TKN_NUMBER,
  TKN_BLOCK,
  TKN_LBRACE,
  TKN_RBRACE,
  TKN_ASSIGNMENT,
  TKN_LPARENT,
  TKN_RPARENT,
  TKN_LBRACKET,
  TKN_RBRACKET,
  TKN_ATTR,
  TKN_INC,
  TKN_DEC,
  TKN_COMMA,
  TKN_COLON,
  TKN_ADD,
  TKN_MINUS,
  TKN_MUL,
  TKN_DIV,
  TKN_MOD,
  TKN_POW,
  TKN_EQ,
  TKN_NE,
  TKN_LT,
  TKN_GT,
  TKN_LE,
  TKN_GE,
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
