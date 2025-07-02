#ifndef __GC_POINTER_ARRAY_H__
#define __GC_POINTER_ARRAY_H__

#include <stddef.h>

#include "GCPointer.h"

struct GCPointer_array {
  struct GCPointer **gc_pointers;
  size_t size;
  size_t capacity;
};

typedef struct GCPointer_array GCPointer_array_t;
typedef GCPointer_array_t *GCPointer_array_ptr;

GCPointer_array_ptr gc_pointer_array_new();
int gc_pointer_array_add(GCPointer_ptr);
void gc_pointer_array_free(GCPointer_array_ptr);

#endif
