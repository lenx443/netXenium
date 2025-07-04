#include <stdlib.h>

#include "ast.h"

AST_Node_t *ast_make_empty() {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_EMPTY;
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
  if (ast->ast_type == AST_CMD) {
    free((void *)ast->cmd.cmd_name);
    for (int i = 0; i < ast->cmd.arg_count; ++i)
      ast_free_arg(ast->cmd.cmd_args[i]);
    free(ast->cmd.cmd_args);
  }
  free(ast);
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
