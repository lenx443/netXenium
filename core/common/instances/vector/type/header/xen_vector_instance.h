#ifndef __XEN_VECTOR_INSTANCE_H__
#define __XEN_VECTOR_INSTANCE_H__

#include <stddef.h>

#include "instance.h"
#include "netxenium/gc_header.h"

struct Xen_Vector_Instance {
  Xen_INSTANCE_HEAD;
  Xen_GCHandle** values;
  size_t capacity;
};

typedef struct Xen_Vector_Instance Xen_Vector;

#endif
