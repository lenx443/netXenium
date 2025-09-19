#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "operators.h"

AST_Node_t *ast_make_empty() {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_EMPTY;
  return node;
}

AST_Node_t *ast_make_string(const char *value) {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_STRING;
  node->string.value = strdup(value);
  return node;
}

AST_Node_t *ast_make_literal(const char *value) {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_LITERAL;
  node->literal.value = strdup(value);
  return node;
}

AST_Node_t *ast_make_property(const char *value) {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_PROPERTY;
  node->property.value = strdup(value);
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

AST_Node_t *ast_make_cmd(const char *cmd_name, AST_Node_t **cmd_args, int arg_count) {
  AST_Node_t *node = malloc(sizeof(AST_Node_t));
  node->ast_type = AST_CMD;
  node->cmd.cmd_name = strdup(cmd_name);
  node->cmd.cmd_args = cmd_args;
  node->cmd.arg_count = arg_count;
  return node;
}

void ast_free(AST_Node_t *ast) {
  if (!ast) return;
  switch (ast->ast_type) {
  case AST_EMPTY: break;
  case AST_STRING: free((void *)ast->string.value);
  case AST_LITERAL: free((void *)ast->literal.value);
  case AST_PROPERTY: free((void *)ast->property.value);
  case AST_ASSIGNMENT:
    free((void *)ast->assignment.lhs.name);
    if (ast->assignment.rhs.type == ASSIGN_STRING)
      free((void *)ast->assignment.rhs.string.value);
    break;
  case AST_CMD:
    free((void *)ast->cmd.cmd_name);
    for (int i = 0; i < ast->cmd.arg_count; ++i)
      ast_free(ast->cmd.cmd_args[i]);
    free(ast->cmd.cmd_args);
    break;
  }
  free(ast);
}

void ast_free_block(AST_Node_t **block, size_t b_count) {
  for (int i = 0; i < b_count; i++) {
    ast_free(block[i]);
  }
  free(block);
}

static void print_indent(int indent) {
  for (int i = 0; i < indent; ++i)
    printf("  ");
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
      print_ast_node(node->cmd.cmd_args[i], indent + 1);
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
