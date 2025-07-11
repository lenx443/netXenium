#include <stdlib.h>

#include "ast.h"

AST_Node_t *ast_make_empty() {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_EMPTY;
  return node;
}

AST_Node_t *ast_make_if_conditional(BoolExprPair_t *condition, AST_Node_t **body,
                                    size_t body_count) {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_IF;
  node->if_conditional.condition = condition;
  node->if_conditional.body = body;
  node->if_conditional.body_count = body_count;
  return node;
}

AST_Node_t *ast_make_cmd(const char *cmd_name, ArgExpr_t **cmd_args, int arg_count) {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_CMD;
  node->cmd.cmd_name = strdup(cmd_name);
  node->cmd.cmd_args = cmd_args;
  node->cmd.arg_count = arg_count;
  return node;
}

BoolExpr_t *ast_make_bool_literal(char *literal) {
  BoolExpr_t *node = malloc(sizeof(BoolExpr_t));
  node->type = BOOL_LITERAL;
  node->content = strdup(literal);
  return node;
}

BoolExpr_t *ast_make_bool_property(char *property) {
  BoolExpr_t *node = malloc(sizeof(BoolExpr_t));
  node->type = BOOL_PROPERTY;
  node->content = strdup(property);
  return node;
}

BoolExpr_t *ast_make_bool_pair(BoolExprPair_t *pair) {
  BoolExpr_t *node = malloc(sizeof(BoolExpr_t));
  node->type = BOOL_PAIR;
  node->pair.c1 = pair->c1;
  node->pair.c2 = pair->c2;
  return node;
}

BoolExprPair_t *ast_make_bool_pair_t(BoolExpr_t *c1, BoolExpr_t *c2) {
  BoolExprPair_t *node = malloc(sizeof(BoolExprPair_t));
  node->c1 = c1;
  node->c2 = c2;
  return node;
}

ArgExpr_t *ast_make_arg_literal(const char *literal) {
  ArgExpr_t *node = malloc(sizeof(ArgExpr_t));
  node->arg_type = ARG_LITERAL;
  node->literal = strdup(literal);
  return node;
}

ArgExpr_t *ast_make_arg_property(const char *property) {
  ArgExpr_t *node = malloc(sizeof(ArgExpr_t));
  node->arg_type = ARG_PROPERTY;
  node->property = strdup(property);
  return node;
}

ArgExpr_t *ast_make_arg_concat(ArgExpr_t **parts, int count) {
  ArgExpr_t *node = malloc(sizeof(ArgExpr_t));
  node->arg_type = ARG_CONCAT;
  node->concat.parts = parts;
  node->concat.count = count;
  return node;
}

void ast_free(AST_Node_t *ast) {
  if (!ast) return;
  if (ast->ast_type == AST_IF) {
    ast_free_bool_pair(ast->if_conditional.condition);
    for (int i = 0; i < ast->if_conditional.body_count; i++) {
      ast_free(ast->if_conditional.body[i]);
    }
  } else if (ast->ast_type == AST_CMD) {
    free((void *)ast->cmd.cmd_name);
    for (int i = 0; i < ast->cmd.arg_count; ++i)
      ast_free_arg(ast->cmd.cmd_args[i]);
    free(ast->cmd.cmd_args);
  }
  free(ast);
}

void ast_free_bool(BoolExpr_t *bool) {
  if (!bool) return;
  if (bool->type == BOOL_LITERAL || bool->type == BOOL_PROPERTY) {
    free(bool->content);
  } else if (bool->type == BOOL_PAIR) {
    ast_free_bool(bool->pair.c1);
    ast_free_bool(bool->pair.c2);
  }
  free(bool);
}

void ast_free_bool_pair(BoolExprPair_t *bool_pair) {
  if (!bool_pair) return;
  ast_free_bool(bool_pair->c1);
  ast_free_bool(bool_pair->c2);
}

void ast_free_arg(ArgExpr_t *arg) {
  if (!arg) return;
  switch (arg->arg_type) {
  case ARG_LITERAL: free((void *)arg->literal); break;
  case ARG_PROPERTY: free((void *)arg->property); break;
  case ARG_CONCAT:
    for (int i = 0; i < arg->concat.count; ++i)
      ast_free_arg(arg->concat.parts[i]);
    free(arg->concat.parts);
    break;
  }
  free(arg);
}
