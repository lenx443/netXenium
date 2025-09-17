#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "operators.h"

AST_Node_t *ast_make_empty() {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_EMPTY;
  return node;
}

AST_Node_t *ast_make_assignment_string(const char *name, const char *value) {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_ASSIGNMENT;
  node->assignment.operator= Xen_Assignment;
  node->assignment.lhs.name = strdup(name);
  node->assignment.rhs.type = ASSIGN_STRING;
  node->assignment.rhs.string.value = strdup(value);
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

ArgExpr_t *ast_make_arg_string(const char *literal) {
  ArgExpr_t *node = malloc(sizeof(ArgExpr_t));
  node->arg_type = ARG_STRING;
  node->string = strdup(literal);
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

void ast_free(AST_Node_t *ast) {
  if (!ast) return;
  if (ast->ast_type == AST_ASSIGNMENT) {
    free((void *)ast->assignment.lhs.name);
    if (ast->assignment.rhs.type == ASSIGN_STRING)
      free((void *)ast->assignment.rhs.string.value);
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

void ast_free_arg(ArgExpr_t *arg) {
  if (!arg) return;
  switch (arg->arg_type) {
  case ARG_STRING: free((void *)arg->string); break;
  case ARG_LITERAL: free((void *)arg->literal); break;
  case ARG_PROPERTY: free((void *)arg->property); break;
  }
  free(arg);
}

static void print_indent(int indent) {
  for (int i = 0; i < indent; ++i)
    printf("  ");
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
  default: print_indent(indent); printf("ArgExpr: Unknown type\n");
  }
}

static void print_ast_node(const AST_Node_t *node, int indent);

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
