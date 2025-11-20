#include <assert.h>

#include "gc_header.h"
#include "gc_heap.h"
#include "instance.h"
#include "xen_gc.h"
#include "xen_typedefs.h"

struct __GC_Heap __gc_heap;

#ifndef __XEN_GC_MAX_ROOTS__
#define __XEN_GC_MAX_ROOTS__ 1024
#endif

#ifndef __XEN_GC_MAX_GRAY__
#define __XEN_GC_MAX_GRAY__ 4096
#endif

struct __GC_Header* __gc_roots[__XEN_GC_MAX_ROOTS__];
struct __GC_Header* __gc_gray_stack[__XEN_GC_MAX_GRAY__];

Xen_size_t __gc_roots_count = 0;
Xen_size_t __gc_gray_stack_count = 0;

void Xen_GC_Push_Root(struct __GC_Header* inst) {
  assert(inst != NULL);
  __gc_roots[__gc_roots_count++] = inst;
}

void Xen_GC_Pop_Root() {
  assert(__gc_roots_count > 0);
  __gc_roots_count--;
}

void Xen_GC_Push_Gray(struct __GC_Header* inst) {
  assert(inst != NULL);
  if (inst->color != GC_WHITE) {
    return;
  }
  inst->color = GC_GRAY;
  __gc_gray_stack[__gc_gray_stack_count++] = inst;
}

struct __GC_Header* Xen_GC_Pop_Gray() {
  assert(__gc_gray_stack_count > 0);
  return __gc_gray_stack[--__gc_gray_stack_count];
}

void Xen_GC_Trace(struct __GC_Header* inst) {
  if (inst->trace) {
    inst->trace(inst);
  }
}

void Xen_GC_Mark() {
  for (Xen_size_t i = 0; i < __gc_roots_count; i++) {
    if (__gc_roots[i]->color == GC_WHITE) {
      Xen_GC_Push_Gray(__gc_roots[i]);
    }
  }
  while (__gc_gray_stack_count > 0) {
    struct __GC_Header* inst = Xen_GC_Pop_Gray();
    Xen_GC_Trace(inst);
    inst->color = GC_BLACK;
  }
}

void Xen_GC_Sweep() {
  struct __GC_Header** curr = &__gc_heap.young;
  while (*curr) {
    if ((*curr)->color == GC_WHITE) {
      struct __GC_Header* unreached = *curr;
      *curr = (*curr)->next;
      __instance_free((struct __Instance*)unreached);
    } else {
      (*curr)->color = GC_WHITE;
      curr = &(*curr)->next;
    }
  }
}
