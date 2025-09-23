#ifndef __AST_H__
#define __AST_H__

#include "instance.h"
#include <stddef.h>

struct AST_Node_s {
  Xen_INSTANCE_HEAD;
  const char *name;
  const char *value;
  int child_count;
  struct AST_Node_s **children;
};

typedef struct AST_Node_s AST_Node_t;

AST_Node_t *ast_node_make(const char *, const char *, int, AST_Node_t **);
int ast_node_push(AST_Node_t *, AST_Node_t *);
void ast_free(AST_Node_t *);

#ifndef NDEBUG
void ast_print(AST_Node_t *);
#endif

#endif
