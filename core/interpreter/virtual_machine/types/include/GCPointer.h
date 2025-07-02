#ifndef __GCPOINTER_H__
#define __GCPOINTER_H__

#include <stddef.h>

struct GCPointer {
  void *gc_ptr;
  size_t gc_size;
  int gc_market;
};

typedef struct GCPointer GCPointer_t;
typedef GCPointer_t *GCPointer_ptr;

GCPointer_ptr gc_pointer_new(void *, size_t);
void gc_pointer_free(GCPointer_ptr);

#endif
