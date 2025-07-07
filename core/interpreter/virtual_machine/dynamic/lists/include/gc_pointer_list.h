#ifndef __GC_POINTER_LIST_H__
#define __GC_POINTER_LIST_H__

#include <stddef.h>

#include "GCPointer.h"

struct GCPointer_node {
  GCPointer_ptr gc_pointer;
  struct GCPointer_node *next;
};

typedef struct GCPointer_node GCPointer_node_t;
typedef GCPointer_node_t *GCPointer_node_ptr;

GCPointer_ptr gc_pointer_list_append(GCPointer_node_ptr *, void *, size_t);
void gc_pointer_list_free(GCPointer_node_ptr);

#endif
