#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

void skip_whitespace(Lexer *lexer) {
  while (1) {
    char c = lexer->src[lexer->pos];
    if (isspace(c)) {
      lexer->pos++;
    } else if (c == '/' && lexer->src[lexer->pos + 1] == '/') {
      while (lexer->src[lexer->pos] && lexer->src[lexer->pos] != '\n')
        lexer->pos++;
    } else {
      break;
    }
  }
}

Lexer_Token lexer_next_token(Lexer *lexer) {
  skip_whitespace(lexer);

  Lexer_Token token = {0};
  token.tkn_type = TKN_EOF;
  token.tkn_text[0] = '\n';

  char c = lexer->src[lexer->pos];
  if (c == '\0') {
    token.tkn_type = TKN_EOF;
    strcpy(token.tkn_text, "<EOF>");
  } else if ((isalpha(c) || c == '_') && lexer->tkn_pos == 0) {
    lexer->tkn_pos++;
    size_t start = lexer->pos;
    while (isalnum(lexer->src[lexer->pos]) || lexer->src[lexer->pos] == '_')
      lexer->pos++;
    size_t len = lexer->pos - start;
    strncpy(token.tkn_text, lexer->src + start, len);
    token.tkn_text[len] = '\0';
    token.tkn_type = TKN_COMMAND;
  } else if (isalnum(c) || c == '_') {
    lexer->tkn_pos++;
    size_t start = lexer->pos;
    while (isalnum(lexer->src[lexer->pos]) || lexer->src[lexer->pos] == '_')
      lexer->pos++;
    size_t len = lexer->pos - start;
    strncpy(token.tkn_text, lexer->src + start, len);
    token.tkn_text[len] = '\0';
    token.tkn_type = TKN_ARGUMENT;
  } else if (c == '$') {
    lexer->pos++;
    lexer->tkn_pos++;
    size_t start = lexer->pos;
    while (isalnum(lexer->src[lexer->pos]) || lexer->src[lexer->pos] == '_')
      lexer->pos++;
    size_t len = lexer->pos - start;
    strncpy(token.tkn_text, lexer->src + start, len);
    token.tkn_text[len] = '\0';
    token.tkn_type = TKN_PROPERTY;
  } else if (c == '\'' || c == '"') {
    lexer->pos++;
    lexer->tkn_pos++;
    size_t start = lexer->pos;
    while (lexer->src[lexer->pos] != '\'' && lexer->src[lexer->pos] != '"') {
      lexer->pos++;
      if (lexer->src[lexer->pos] == '\0') {
        lexer->pos++;
        lexer->tkn_pos++;
        token.tkn_type = TKN_UNDEFINED;
        strcpy(token.tkn_text, "<UNDEF>");
        return token;
      }
    }
    size_t len = lexer->pos - start;
    strncpy(token.tkn_text, lexer->src + start, len);
    token.tkn_text[len] = '\0';
    token.tkn_type = TKN_STRING;
    lexer->pos++;
  } else if (c == '@') {
    lexer->pos++;
    lexer->tkn_pos++;
    if (lexer->src[lexer->pos] == '@') {
      lexer->pos++;
      token.tkn_type = TKN_CONCAT;
      strcpy(token.tkn_text, "@@");
    } else {
      token.tkn_text[0] = '@';
      token.tkn_text[1] = lexer->src[lexer->pos];
      token.tkn_text[2] = '\0';
      lexer->pos++;
      lexer->tkn_pos++;
      token.tkn_type = TKN_UNDEFINED;
    }
  } else if (c == '=') {
    lexer->pos++;
    lexer->tkn_pos++;
    token.tkn_type = TKN_EQUAL;
    strcat(token.tkn_text, "=");
  } else {
    token.tkn_text[0] = lexer->src[lexer->pos];
    token.tkn_text[1] = '\0';
    lexer->pos++;
    lexer->tkn_pos++;
    token.tkn_type = TKN_UNDEFINED;
  }
  return token;
}
