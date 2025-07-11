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
    ast_free_block(ast->if_conditional.body, ast->if_conditional.body_count);
  } else if (ast->ast_type == AST_CMD) {
    free((void *)ast->cmd.cmd_name);
    for (int i = 0; i < ast->cmd.arg_count; ++i)
      ast_free_arg(ast->cmd.cmd_args[i]);
    free(ast->cmd.cmd_args);
  }
  free(ast);
}

void ast_free_block(AST_Node_t **block, size_t b_count) {
  for (int i = 0; i < b_count; i++) {
    ast_free(block[i]);
  }
  free(block);
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

static void print_indent(int indent) {
  for (int i = 0; i < indent; ++i)
    printf("  ");
}

static void print_bool_expr(const BoolExpr_t *expr, int indent);

static void print_bool_pair(const BoolExprPair_t *pair, int indent) {
  print_indent(indent);
  printf("BoolPair:\n");
  if (pair) {
    print_bool_expr(pair->c1, indent + 1);
    print_bool_expr(pair->c2, indent + 1);
  }
}

static void print_bool_expr(const BoolExpr_t *expr, int indent) {
  if (!expr) {
    print_indent(indent);
    printf("BoolExpr: NULL\n");
    return;
  }
  switch (expr->type) {
  case BOOL_LITERAL:
    print_indent(indent);
    printf("BoolLiteral: \"%s\"\n", expr->content ? expr->content : "NULL");
    break;
  case BOOL_PROPERTY:
    print_indent(indent);
    printf("BoolProperty: \"%s\"\n", expr->content ? expr->content : "NULL");
    break;
  case BOOL_PAIR:
    print_indent(indent);
    printf("BoolPair:\n");
    print_bool_expr(expr->pair.c1, indent + 1);
    print_bool_expr(expr->pair.c2, indent + 1);
    break;
  default: print_indent(indent); printf("BoolExpr: Unknown type\n");
  }
}

static void print_arg_expr(const ArgExpr_t *arg, int indent) {
  if (!arg) {
    print_indent(indent);
    printf("ArgExpr: NULL\n");
    return;
  }
  switch (arg->arg_type) {
  case ARG_LITERAL:
    print_indent(indent);
    printf("ArgLiteral: \"%s\"\n", arg->literal ? arg->literal : "NULL");
    break;
  case ARG_PROPERTY:
    print_indent(indent);
    printf("ArgProperty: \"%s\"\n", arg->property ? arg->property : "NULL");
    break;
  case ARG_CONCAT:
    print_indent(indent);
    printf("ArgConcat:\n");
    for (int i = 0; i < arg->concat.count; ++i) {
      print_arg_expr(arg->concat.parts[i], indent + 1);
    }
    break;
  default: print_indent(indent); printf("ArgExpr: Unknown type\n");
  }
}

static void print_ast_node(const AST_Node_t *node, int indent);

static void print_ast_block(AST_Node_t **body, size_t body_count, int indent) {
  for (size_t i = 0; i < body_count; ++i) {
    print_ast_node(body[i], indent);
  }
}

static void print_ast_node(const AST_Node_t *node, int indent) {
  if (!node) {
    print_indent(indent);
    printf("AST_Node: NULL\n");
    return;
  }
  switch (node->ast_type) {
  case AST_EMPTY:
    print_indent(indent);
    printf("AST_EMPTY\n");
    break;
  case AST_CMD:
    print_indent(indent);
    printf("AST_CMD: \"%s\"\n", node->cmd.cmd_name ? node->cmd.cmd_name : "NULL");
    for (int i = 0; i < node->cmd.arg_count; ++i) {
      print_arg_expr(node->cmd.cmd_args[i], indent + 1);
    }
    break;
  case AST_IF:
    print_indent(indent);
    printf("AST_IF:\n");
    print_indent(indent + 1);
    printf("Condition:\n");
    if (node->if_conditional.condition)
      print_bool_pair(node->if_conditional.condition, indent + 2);
    else
      print_indent(indent + 2), printf("NULL\n");
    print_indent(indent + 1);
    printf("Body:\n");
    print_ast_block(node->if_conditional.body, node->if_conditional.body_count,
                    indent + 2);
    break;
  default: print_indent(indent); printf("AST_Node: Unknown type\n");
  }
}

void ast_print(const AST_Node_t *node) { print_ast_node(node, 0); }

void ast_print_block(AST_Node_t **nodes, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    printf("Node %zu:\n", i);
    ast_print(nodes[i]);
    printf("\n");
  }
}
