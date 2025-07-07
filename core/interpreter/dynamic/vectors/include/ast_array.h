#ifndef __AST_ARRAY_H__
#define __AST_ARRAY_H__

#include <stdint.h>

#include "ast.h"

struct AST_Array {
  AST_Node_t **ast_array;
  size_t ast_count;
  size_t ast_capacity;
};

typedef struct AST_Array AST_Array_t;
typedef AST_Array_t *AST_Array_ptr;

AST_Array_ptr ast_array_new();
int ast_array_add(AST_Array_ptr, AST_Node_t *);
void ast_array_free(AST_Array_ptr);

#endif
