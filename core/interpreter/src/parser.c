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
    parser_next(p);
    return parser_stmt(p, ast);
  }
  return 0;
}

int parser_block(Parser *p, AST_Node_t ***ast_array, size_t *block_count) {
  parser_next(p);
  while (p->token.tkn_type != TKN_BLOCK) {
    if (p->token.tkn_type == TKN_NEWLINE)
      parser_next(p);
    else
      return 0;
  }
  parser_next(p);
  while (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
  }
  *ast_array = NULL;
  size_t b_count = 0, b_cap = 0;
  if (p->token.tkn_type == TKN_LBRACE) {
    parser_next(p);
    while (p->token.tkn_type != TKN_RBRACE) {
      if (b_count >= b_cap) {
        int new_cap = b_cap == 0 ? 4 : b_cap * 2;
        AST_Node_t **temp = realloc(*ast_array, sizeof(AST_Node_t *) * new_cap);
        if (!temp) {
          error("Memoria insuficiente");
          return 0;
        }
        *ast_array = temp;
        b_cap = new_cap;
      }
      if (!parser_stmt(p, &(*ast_array)[b_count])) {
        ast_free_block(*ast_array, b_count);
        return 0;
      }
      b_count++;
    }
    parser_next(p);
  } else {
    *ast_array = malloc(sizeof(AST_Node_t *));
    if (*ast_array == NULL) {
      error("Memoria insuficiente");
      return 0;
    }
    if (!parser_stmt(p, *ast_array)) {
      free(*ast_array);
      return 0;
    }
    b_count = 1;
  }
  *block_count = b_count;
  return 1;
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
  size_t count;
  AST_Node_t **body;
  if (!parser_block(p, &body, &count)) {
    ast_free_bool(b1);
    ast_free_bool(b2);
    return 0;
  }
  *ast = ast_make_if_conditional(ast_make_bool_pair(b1, b2), body, count);
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
