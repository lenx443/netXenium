#ifndef __LEXER_H__
#define __LEXER_H__

#include <stddef.h>

#include "source_file.h"
#include "xen_typedefs.h"

#define LXR_TOKEN_SIZE 200

typedef enum {
  TKN_EOF = 0,
  TKN_UNDEFINED,
  TKN_NEWLINE,
  TKN_IDENTIFIER,
  TKN_KEYWORD,
  TKN_HAS,
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
  TKN_COMMA,
  TKN_COLON,
  TKN_QUESTION,
  TKN_DOUBLE_QUESTION,
  TKN_ARROW,
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
  char tkn_text[LXR_TOKEN_SIZE];
  Xen_Source_Address sta;
} Lexer_Token;

typedef struct {
  Xen_size_t sf_id;
  Xen_size_t line;
  Xen_size_t column;
  Xen_size_t pos;
  Xen_size_t start_line;
  Xen_size_t start_column;
} Lexer;

void skip_whitespace(Lexer*);
Lexer_Token lexer_next_token(Lexer*);

#endif
