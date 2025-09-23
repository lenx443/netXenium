#ifndef __XEN_AST_INSTANCE_H__
#define __XEN_AST_INSTANCE_H__

#include <stddef.h>

#include "instance.h"

struct Xen_AST_Node_Instance {
  Xen_INSTANCE_HEAD;
  const char *name;
  const char *value;
  Xen_Instance *children;
};

typedef struct Xen_AST_Node_Instance Xen_AST_Node;

#endif
