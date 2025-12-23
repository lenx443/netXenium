#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "gc_header.h"
#include "gc_heap.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_typedefs.h"

static struct __GC_Heap __gc_heap = {
    .young = NULL,
    .old = NULL,
    .roots_count = 0,
    .gray_stack_count = 0,
    .total_bytes = 0,
    .threshold = 1024 * 1024,
    .pressure = 0,
    .started = 1,
};

void Xen_GC_GetReady(void) {
  xen_globals->gc_heap = &__gc_heap;
}

struct __GC_Header* Xen_GC_New(Xen_size_t size,
                               void (*fn_trace)(struct __GC_Header*),
                               void (*fn_destroy)(struct __GC_Header**)) {
  struct __GC_Header* h = (struct __GC_Header*)Xen_Alloc(size);
  Xen_GC_Push_Root(h);

  memset(h, 0, size);
  h->color = GC_WHITE;
  h->generation = GC_YOUNG;
  h->size = size;

  h->prev = NULL;
  h->next = xen_globals->gc_heap->young;
  if (xen_globals->gc_heap->young) {
    xen_globals->gc_heap->young->prev = h;
  }
  xen_globals->gc_heap->young = h;

  xen_globals->gc_heap->total_bytes += size;
  xen_globals->gc_heap->pressure += size;

  if (xen_globals->gc_heap->started) {
    if (xen_globals->gc_heap->pressure > xen_globals->gc_heap->threshold) {
      Xen_GC_Collect();
    }
  }
  h->trace = fn_trace;
  h->destroy = fn_destroy;
  Xen_GC_Pop_Root();
  return h;
}

void Xen_GC_Collect(void) {
  Xen_GC_Mark();
  Xen_GC_Sweep();
  xen_globals->gc_heap->pressure = 0;
}

void Xen_GC_Push_Root(struct __GC_Header* inst) {
  assert(xen_globals->gc_heap->roots_count < __XEN_GC_MAX_ROOTS__);
  assert(inst != NULL);
  xen_globals->gc_heap->roots[xen_globals->gc_heap->roots_count++] = inst;
}

void Xen_GC_Pop_Root(void) {
  assert(xen_globals->gc_heap->roots_count > 0);
  xen_globals->gc_heap->roots_count--;
}

void Xen_GC_Pop_Roots(Xen_size_t count) {
  for (Xen_size_t i = 0; i < count; i++) {
    Xen_GC_Pop_Root();
  }
}

void Xen_GC_Push_Gray(struct __GC_Header* inst) {
  assert(xen_globals->gc_heap->gray_stack_count < __XEN_GC_MAX_GRAY__);
  assert(inst != NULL);
  if (inst->color != GC_WHITE) {
    return;
  }
  inst->color = GC_GRAY;
  xen_globals->gc_heap->gray_stack[xen_globals->gc_heap->gray_stack_count++] =
      inst;
}

struct __GC_Header* Xen_GC_Pop_Gray(void) {
  assert(xen_globals->gc_heap->gray_stack_count > 0);
  return xen_globals->gc_heap
      ->gray_stack[--xen_globals->gc_heap->gray_stack_count];
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

void Xen_GC_Mark(void) {
  for (Xen_size_t i = 0; i < xen_globals->gc_heap->roots_count; i++) {
    if (xen_globals->gc_heap->roots[i]->color == GC_WHITE) {
      Xen_GC_Push_Gray(xen_globals->gc_heap->roots[i]);
    }
  }
  while (xen_globals->gc_heap->gray_stack_count > 0) {
    struct __GC_Header* inst = Xen_GC_Pop_Gray();
    Xen_GC_Trace(inst);
    inst->color = GC_BLACK;
  }
}

void Xen_GC_Sweep(void) {
  struct __GC_Header* curr = xen_globals->gc_heap->young;

  while (curr) {
    struct __GC_Header* next = curr->next;

    if (curr->color == GC_WHITE) {
      if (curr->prev) {
        curr->prev->next = curr->next;
      }
      if (curr->next) {
        curr->next->prev = curr->prev;
      }

      if (xen_globals->gc_heap->young == curr) {
        xen_globals->gc_heap->young = curr->next;
      }

      xen_globals->gc_heap->total_bytes -= curr->size;

      curr->destroy(&curr);

    } else {
      curr->color = GC_WHITE;
    }

    curr = next;
  }
}

void Xen_GC_Shutdown(void) {
  Xen_GC_Collect();
  assert(xen_globals->gc_heap->pressure == 0);
  assert(xen_globals->gc_heap->total_bytes == 0);
}

void Xen_GC_Write_Field(struct __GC_Header* parent, struct __GC_Header** field,
                        struct __GC_Header* child) {
  assert(child != NULL);
  *field = child;
  if (parent && parent->color == GC_BLACK && child->color == GC_WHITE) {
    Xen_GC_Push_Gray(child);
  }
}

void Xen_GC_Start(void) {
  xen_globals->gc_heap->started = 1;
}

void Xen_GC_Stop(void) {
  xen_globals->gc_heap->started = 0;
}
