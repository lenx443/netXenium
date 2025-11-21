#include <assert.h>
#include <stddef.h>

#include "gc_header.h"
#include "gc_heap.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_typedefs.h"

struct __GC_Heap __gc_heap = {
    .young = NULL,
    .old = NULL,
    .total_bytes = 0,
    .threshold = 1024 * 1024,
    .pressure = 0,
};

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

struct __GC_Header* Xen_GC_New(Xen_size_t size,
                               void (*fn_trace)(struct __GC_Header*),
                               void (*fn_destroy)(struct __GC_Header**)) {
  assert(size >= sizeof(struct __GC_Header));
  struct __GC_Header* h = (struct __GC_Header*)Xen_Alloc(size);

  h->color = GC_WHITE;
  h->generation = GC_YOUNG;
  h->age = 0;
  h->size = size;
  h->trace = fn_trace;
  h->destroy = fn_destroy;

  h->prev = NULL;
  h->next = __gc_heap.young;
  if (__gc_heap.young) {
    __gc_heap.young->prev = h;
  }
  __gc_heap.young = h;

  __gc_heap.total_bytes += size;
  __gc_heap.pressure += size;

  if (__gc_heap.pressure > __gc_heap.threshold) {
    Xen_GC_Push_Root(h);
    Xen_GC_Collect();
    Xen_GC_Pop_Root();
  }
  return h;
}

void Xen_GC_Collect() {
  Xen_GC_Mark();
  Xen_GC_Sweep();
  __gc_heap.pressure = 0;
}

void Xen_GC_Push_Root(struct __GC_Header* inst) {
  assert(__gc_roots_count < __XEN_GC_MAX_ROOTS__);
  assert(inst != NULL);
  __gc_roots[__gc_roots_count++] = inst;
}

void Xen_GC_Pop_Root() {
  assert(__gc_roots_count > 0);
  __gc_roots_count--;
}

void Xen_GC_Pop_Roots(Xen_size_t count) {
  for (Xen_size_t i = 0; i < count; i++) {
    Xen_GC_Pop_Root();
  }
}

void Xen_GC_Push_Gray(struct __GC_Header* inst) {
  assert(__gc_gray_stack_count < __XEN_GC_MAX_GRAY__);
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

void Xen_GC_Trace_GCHeader(struct __GC_Header* h) {
  assert(h != NULL);
  if (h->color == GC_WHITE) {
    Xen_GC_Push_Gray(h);
  }
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
  struct __GC_Header* curr = __gc_heap.young;

  while (curr) {
    struct __GC_Header* next = curr->next;

    if (curr->color == GC_WHITE) {
      if (curr->prev) {
        curr->prev->next = curr->next;
      }
      if (curr->next) {
        curr->next->prev = curr->prev;
      }

      if (__gc_heap.young == curr) {
        __gc_heap.young = curr->next;
      }

      __gc_heap.total_bytes -= curr->size;

      curr->destroy(&curr);

    } else {
      curr->color = GC_WHITE;
    }

    curr = next;
  }
}
