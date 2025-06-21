#ifndef __PARSER_H__
#define __PARSER_H__

#include "commands.h"
#include "lexer.h"
#include "properties.h"
#include "raw_arguments.h"

typedef struct {
  Lexer *lexer;
  Lexer_Token token;
} Parser;

typedef struct {
  enum {
    CDG_NULL = 0,
    CDG_COMMAND,
    CDG_ASIGN_PROP,
  } cdg_type;
  union {
    struct {
      Command *command;
      Raw_Arguments args;
    } command;
    struct {
      prop_struct *prop;
      Raw_Arguments new_value;
    } assign_prop;
  } cdg_value;
} Parser_Code;

void parser_next(Parser *);
int parser_stmt(Parser *, Parser_Code *);
void parser_code_free(Parser_Code *);

#endif
