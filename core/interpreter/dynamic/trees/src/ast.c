#include <stdlib.h>

#include "ast.h"

AST_Node_t *ast_node_make(const char *name, const char *value, int child_count,
                          AST_Node_t **children) {
  AST_Node_t *ast = malloc(sizeof(AST_Node_t));
  ast->name = name;
  ast->value = value;
  ast->child_count = child_count;
  ast->children = children;
  return ast;
}

int ast_node_push(AST_Node_t *ast, AST_Node_t *child) {
  ast->children = realloc(ast->children, sizeof(AST_Node_t *) * (ast->child_count + 1));
  ast->children[ast->child_count++] = child;
  return 1;
}

void ast_free(AST_Node_t *ast) {
  if (!ast) return;
  if (ast->name) free((void *)ast->name);
  if (ast->value) free((void *)ast->value);
  if (ast->children) {
    for (int i = 0; i < ast->child_count; i++) {
      ast_free(ast->children[i]);
    }
    free(ast->children);
  }
  free(ast);
}

#ifndef NDEBUG
static void __print_ast(AST_Node_t *node, int level) {
  if (!node) return;

  // Imprimir sangría según el nivel
  for (int i = 0; i < level; i++) {
    printf("  "); // dos espacios por nivel
  }

  // Imprimir el nodo
  if (node->value)
    printf("%s: %s\n", node->name, node->value);
  else
    printf("%s\n", node->name);

  // Llamada recursiva para los hijos
  for (int i = 0; i < node->child_count; i++) {
    __print_ast(node->children[i], level + 1);
  }
}

void ast_print(AST_Node_t *node) { __print_ast(node, 0); }
#endif
