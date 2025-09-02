#include <stdlib.h>

#include "ast.h"
#include "ast_array.h"
#include "logs.h"

#define error(msg, ...) log_add(NULL, ERROR, "AST Array", msg, ##__VA_ARGS__)

AST_Array_ptr ast_array_new() {
  AST_Array_ptr ast_array = malloc(sizeof(AST_Array_t));
  if (!ast_array) {
    error("Memoria insuficiente");
    return NULL;
  }
  ast_array->ast_array = NULL;
  ast_array->ast_count = 0;
  ast_array->ast_capacity = 0;
  return ast_array;
}

int ast_array_add(AST_Array_ptr ast_array, AST_Node_t *ast) {
  if (!ast_array || !ast) {
    error("No se pudo agregar el arbol ast al arreglo");
    return 0;
  }
  if (ast_array->ast_count >= ast_array->ast_capacity) {
    int new_capacity = ast_array->ast_capacity > 0 ? ast_array->ast_capacity * 2 : 4;
    AST_Node_t **temp =
        realloc(ast_array->ast_array, sizeof(AST_Node_t *) * new_capacity);
    if (!temp) {
      error("Memoria insuficiente");
      return 0;
    }
    ast_array->ast_array = temp;
    ast_array->ast_capacity = new_capacity;
  }
  ast_array->ast_array[ast_array->ast_count++] = ast;
  return 1;
}

void ast_array_free(AST_Array_ptr ast_array) {
  if (!ast_array) return;
  for (int i = 0; i < ast_array->ast_count; i++) {
    ast_free(ast_array->ast_array[i]);
  }
  free(ast_array->ast_array);
  free(ast_array);
}
