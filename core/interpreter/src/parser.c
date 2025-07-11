#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "logs.h"
#include "parser.h"

#define error(msg, ...) log_add(NULL, ERROR, "Parser", msg, ##__VA_ARGS__)

void parser_next(Parser *p) { p->token = lexer_next_token(p->lexer); }

int parser_stmt(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type == TKN_IDENTIFIER) {
    char *identifier_name = strdup(p->token.tkn_text);
    if (strcmp(identifier_name, "if") == 0) { return parser_if_cindional(p, ast); }
    parser_next(p);
    ArgExpr_t **args = NULL;
    int arg_count = 0, arg_cap = 0;
    while (p->token.tkn_type != TKN_UNDEFINED && p->token.tkn_type != TKN_EOF) {
      if (p->token.tkn_type == TKN_NEWLINE) {
        *ast = ast_make_cmd(identifier_name, args, arg_count);
        free(identifier_name);
        parser_next(p);
        return 1;
      }
      ArgExpr_t *arg = parser_concat(p);
      if (!arg) {
        free(identifier_name);
        return 0;
      }
      if (arg_count == arg_cap) {
        arg_cap = arg_cap ? arg_cap * 2 : 4;
        args = realloc(args, sizeof(ArgExpr_t *) * arg_cap);
      }
      args[arg_count++] = arg;
    }
    *ast = ast_make_cmd(identifier_name, args, arg_count);
    free(identifier_name);
    return 1;
  } else if (p->token.tkn_type == TKN_NEWLINE) {
    *ast = ast_make_empty();
    parser_next(p);
    return 1;
  }
  return 0;
}

int parser_if_cindional(Parser *p, AST_Node_t **ast) {
  parser_next(p);
  BoolExpr_t *b1;
  BoolExpr_t *b2;
  switch (p->token.tkn_type) {
  case TKN_STRING: b1 = ast_make_bool_literal(p->token.tkn_text); break;
  case TKN_PROPERTY: b1 = ast_make_bool_property(p->token.tkn_text); break;
  default: error("Invalid token"); return 0;
  }
  parser_next(p);
  switch (p->token.tkn_type) {
  case TKN_STRING: b2 = ast_make_bool_literal(p->token.tkn_text); break;
  case TKN_PROPERTY: b2 = ast_make_bool_property(p->token.tkn_text); break;
  default:
    error("Invalid token");
    ast_free_bool(b1);
    return 0;
  }
  // ast_make_if_conditional(ast_make_bool_pair_t(b1, b2), AST_Node_t **, size_t);
  return 1;
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
