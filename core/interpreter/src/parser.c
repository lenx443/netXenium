#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "logs.h"
#include "parser.h"

#define error(msg, ...) log_add(NULL, ERROR, "Parser", msg, ##__VA_ARGS__)

void parser_next(Parser *p) { p->token = lexer_next_token(p->lexer); }

Lexer_Token_Type parser_peek(Parser *p) {
  int start = p->lexer->pos;
  Lexer_Token_Type token = lexer_next_token(p->lexer).tkn_type;
  p->lexer->pos = start;
  return token;
}

int parser_stmt(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type == TKN_IDENTIFIER) {
    char *identifier_name = strdup(p->token.tkn_text);
    parser_next(p);
    if (p->token.tkn_type == TKN_ASSIGNMENT) {
      parser_next(p);
      if (!parser_assignment_rhs(identifier_name, p, ast)) {
        free(identifier_name);
        return 0;
      }
      free(identifier_name);
      return 1;
    }
    AST_Node_t **args = NULL;
    int arg_count = 0, arg_cap = 0;
    while (p->token.tkn_type != TKN_UNDEFINED && p->token.tkn_type != TKN_EOF) {
      if (p->token.tkn_type == TKN_NEWLINE) {
        *ast = ast_make_cmd(identifier_name, args, arg_count);
        free(identifier_name);
        parser_next(p);
        return 1;
      }
      AST_Node_t *arg = parser_arg(p);
      if (!arg) {
        free(identifier_name);
        return 0;
      }
      if (arg_count == arg_cap) {
        arg_cap = arg_cap ? arg_cap * 2 : 4;
        args = realloc(args, sizeof(AST_Node_t *) * arg_cap);
      }
      args[arg_count++] = arg;
    }
    *ast = ast_make_cmd(identifier_name, args, arg_count);
    free(identifier_name);
    return 1;
  } else if (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
    return parser_stmt(p, ast);
  } else if (p->token.tkn_type == TKN_EOF) {
    *ast = ast_make_empty();
    return 0;
  }
  error("El token '%s' es invalido. Use `help` para mas información", p->token.tkn_text);
  *ast = ast_make_empty();
  return 0;
}

int parser_string(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type == TKN_STRING) {
    *ast = ast_make_string(p->token.tkn_text);
    return 1;
  }
  return 0;
}

int parser_literal(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type == TKN_IDENTIFIER) {
    *ast = ast_make_literal(p->token.tkn_text);
    return 1;
  }
  return 0;
}

int parser_property(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type == TKN_PROPERTY) {
    *ast = ast_make_property(p->token.tkn_text);
    return 1;
  }
  return 0;
}

int parser_assignment_rhs(const char *name, Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type == TKN_STRING) {
    *ast = ast_make_assignment_string(name, p->token.tkn_text);
    parser_next(p);
    if (p->token.tkn_type != TKN_NEWLINE && p->token.tkn_type != TKN_EOF) { return 0; }
    return 1;
  }
  return 0;
}

int parser_block(Parser *p, AST_Node_t ***ast_array, size_t *block_count) {
  parser_next(p);
  while (p->token.tkn_type != TKN_BLOCK) {
    if (p->token.tkn_type == TKN_NEWLINE)
      parser_next(p);
    else {
      error("El token '%s' es invalido.", p->token.tkn_text);
      return 0;
    }
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
      while (p->token.tkn_type == TKN_NEWLINE) {
        parser_next(p);
      }
      if (p->token.tkn_type == TKN_RBRACE) break;
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

AST_Node_t *parser_arg(Parser *p) {
  if (p->token.tkn_type == TKN_STRING) {
    AST_Node_t *node = ast_make_string(p->token.tkn_text);
    parser_next(p);
    return node;
  }
  if (p->token.tkn_type == TKN_IDENTIFIER) {
    AST_Node_t *node = ast_make_literal(p->token.tkn_text);
    parser_next(p);
    return node;
  }
  if (p->token.tkn_type == TKN_PROPERTY) {
    AST_Node_t *node = ast_make_property(p->token.tkn_text);
    parser_next(p);
    return node;
  }
  parser_next(p);
  return NULL;
}
