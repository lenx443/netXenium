#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

void parser_next(Parser *p) { p->token = lexer_next_token(p->lexer); }

int parser_stmt(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type == TKN_IDENTIFIER) {
    char *cmd_name = strdup(p->token.tkn_text);
    parser_next(p);
    ArgExpr_t **args = NULL;
    int arg_count = 0, arg_cap = 0;
    while (p->token.tkn_type != TKN_UNDEFINED && p->token.tkn_type != TKN_EOF) {
      if (p->token.tkn_type == TKN_NEWLINE) {
        puts("Parsing new line");
        free(cmd_name);
        parser_next(p);
        return 1;
      }
      ArgExpr_t *arg = parser_concat(p);
      if (!arg) goto error_exit;
      if (arg_count == arg_cap) {
        arg_cap = arg_cap ? arg_cap * 2 : 4;
        args = realloc(args, sizeof(ArgExpr_t *) * arg_cap);
      }
      args[arg_count++] = arg;
    }
    *ast = ast_make_cmd(cmd_name, args, arg_count);
    free(cmd_name);
    return 1;
  error_exit:
    free(cmd_name);
    return 0;
  }
  return 0;
}

ArgExpr_t *parser_concat(Parser *p) {
  ArgExpr_t **parts = NULL;
  int count = 0, cap = 0;
  do {
    ArgExpr_t *part = parser_arg(p);
    if (!part) break;
    if (count == cap) {
      cap = cap ? cap * 2 : 4;
      parts = realloc(parts, sizeof(ArgExpr_t *) * cap);
    }
    parts[count++] = part;
    if (p->token.tkn_type == TKN_CONCAT)
      parser_next(p);
    else { break; }
  } while (1);
  if (count == 1) {
    ArgExpr_t *single = parts[0];
    free(parts);
    return single;
  }
  return ast_make_arg_concat(parts, count);
}

ArgExpr_t *parser_arg(Parser *p) {
  if (p->token.tkn_type == TKN_IDENTIFIER || p->token.tkn_type == TKN_STRING) {
    ArgExpr_t *node = ast_make_arg_literal(p->token.tkn_text);
    parser_next(p);
    return node;
  }
  if (p->token.tkn_type == TKN_PROPERTY) {
    ArgExpr_t *node = ast_make_arg_property(p->token.tkn_text);
    parser_next(p);
    return node;
  }
  parser_next(p);
  return NULL;
}
