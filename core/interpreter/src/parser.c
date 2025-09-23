#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "instance.h"
#include "lexer.h"
#include "logs.h"
#include "parser.h"
#include "xen_ast.h"
#include "xen_nil.h"

#define error(msg, ...) log_add(NULL, ERROR, "Parser", msg, ##__VA_ARGS__)

static int parser_string(Parser *, AST_Node_t **);
static int parser_literal(Parser *, AST_Node_t **);
static int parser_property(Parser *, AST_Node_t **);
static Xen_Instance *parser_expr(Parser *);
static int parser_primary(Parser *, AST_Node_t **);
static Xen_Instance *parser_assignment(Parser *);
static int parser_block(Parser *, AST_Node_t ***, size_t *);
static AST_Node_t *parser_arg(Parser *);

void parser_next(Parser *p) { p->token = lexer_next_token(p->lexer); }

static Lexer_Token_Type parser_peek(Parser *p) {
  int start = p->lexer->pos;
  Lexer_Token_Type token = lexer_next_token(p->lexer).tkn_type;
  p->lexer->pos = start;
  return token;
}

Xen_Instance *parser_stmt(Parser *p) {
  if (p->token.tkn_type == TKN_IDENTIFIER) {
    char *identifier_name = strdup(p->token.tkn_text);

    if (parser_peek(p) == TKN_ASSIGNMENT) {
      Xen_Instance *assignm = parser_assignment(p);
      if_nil_eval(assignm) {
        free(identifier_name);
        return nil;
      }
      free(identifier_name);
      return assignm;
    }

    parser_next(p);
    if (p->token.tkn_type == TKN_LPARENT) {
      parser_next(p);

      Xen_Instance *cmd = Xen_AST_Node_New("cmd", identifier_name);
      if_nil_eval(cmd) {
        free(identifier_name);
        return nil;
      }
      while (p->token.tkn_type != TKN_RPARENT) {
        Xen_Instance *arg = parser_expr(p);
        if_nil_eval(arg) {
          Xen_DEL_REF(cmd);
          free(identifier_name);
          return nil;
        }
        if (!Xen_AST_Node_Push_Child(cmd, arg)) {}
      }

      *ast = ast_node_make(strdup("cmd"), strdup(identifier_name), arg_count, args);
      free(identifier_name);
      parser_next(p);
      return 1;
    }

    free(identifier_name);
  } else if (p->token.tkn_type == TKN_NEWLINE) {
    parser_next(p);
    return parser_stmt(p, ast);
  } else if (p->token.tkn_type == TKN_EOF) {
    *ast = ast_node_make(strdup("empty"), NULL, 0, NULL);
    return 0;
  }

  error("El token '%s' es invalido. Use `help` para mas información", p->token.tkn_text);
  *ast = ast_node_make(strdup("empty"), NULL, 0, NULL);
  return 0;
}

int parser_string(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type != TKN_STRING) return 0;
  *ast = ast_node_make(strdup("string"), strdup(p->token.tkn_text), 0, NULL);
  parser_next(p);
  return 1;
}

int parser_literal(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type != TKN_IDENTIFIER) return 0;
  *ast = ast_node_make(strdup("literal"), strdup(p->token.tkn_text), 0, NULL);
  parser_next(p);
  return 1;
}

int parser_property(Parser *p, AST_Node_t **ast) {
  if (p->token.tkn_type != TKN_PROPERTY) return 0;
  *ast = ast_node_make(strdup("property"), strdup(p->token.tkn_text), 0, NULL);
  parser_next(p);
  return 1;
}

int parser_expr(Parser *p, AST_Node_t **ast) {
  AST_Node_t *primary = NULL;
  if (!parser_primary(p, &primary)) return 0;
  AST_Node_t **children = malloc(sizeof(AST_Node_t *));
  children[0] = primary;
  *ast = ast_node_make(strdup("expr"), NULL, 1, children);
  return 1;
}

int parser_primary(Parser *p, AST_Node_t **ast) {
  AST_Node_t *value = NULL;
  if (p->token.tkn_type == TKN_STRING) {
    if (!parser_string(p, &value)) { return 0; }
  } else if (p->token.tkn_type == TKN_IDENTIFIER) {
    if (!parser_literal(p, &value)) { return 0; }
  } else if (p->token.tkn_type == TKN_PROPERTY) {
    if (parser_property(p, &value)) { return 0; }
  } else {
    return 0;
  }
  AST_Node_t **children = malloc(sizeof(AST_Node_t *));
  children[0] = value;
  *ast = ast_node_make(strdup("primary"), NULL, 1, children);
  return 1;
}

int parser_assignment(Parser *p, AST_Node_t **ast) {
  AST_Node_t *lhs =
      ast_node_make(strdup("literal"), strdup(p->token.tkn_text), 0, NULL); // LHS
  parser_next(p);

  if (p->token.tkn_type == TKN_ASSIGNMENT) {
    const char *operator= "="; // por defecto
    parser_next(p);

    AST_Node_t *rhs = NULL;
    if (p->token.tkn_type == TKN_STRING) {
      parser_string(p, &rhs);
    } else if (p->token.tkn_type == TKN_IDENTIFIER) {
      parser_literal(p, &rhs);
    } else if (p->token.tkn_type == TKN_PROPERTY) {
      parser_property(p, &rhs);
    } else {
      ast_free(lhs);
      return 0;
    }

    // Crear nodo assignment con operador implícito
    AST_Node_t **children = malloc(sizeof(AST_Node_t *) * 2);
    children[0] = lhs;
    children[1] = rhs; // RHS
    *ast = ast_node_make(strdup("assignment"), strdup(operator), 2, children);

    parser_next(p);
    return 1;
  }

  ast_free(lhs);
  return 0;
}

AST_Node_t *parser_arg(Parser *p) {
  AST_Node_t *node = NULL;

  if (p->token.tkn_type == TKN_STRING) {
    parser_string(p, &node);
  } else if (p->token.tkn_type == TKN_IDENTIFIER) {
    parser_literal(p, &node);
  } else if (p->token.tkn_type == TKN_PROPERTY) {
    parser_property(p, &node);
  } else {
    parser_next(p);
    return NULL;
  }

  return node;
}
