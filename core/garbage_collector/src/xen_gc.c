#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "gc_header.h"
#include "gc_heap.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_typedefs.h"

#define XEN_GC_PROMOTION_AGE 2
#define XEN_GC_MAJOR_EVERY_MINORS 8

static struct __GC_Heap __gc_heap = {
    .young = NULL,
    .old = NULL,
    .roots_count = 0,
    .gray_stack_count = 0,
    .young_bytes = 0,
    .old_bytes = 0,
    .minor_collections = 0,
    .major_collections = 0,
    .minor_threshold = 1024 * 1024,
    .major_threshold = 4 * 1024 * 1024,
    .major_pressure = 0,
    .total_bytes = 0,
    .pressure = 0,
    .started = 1,
    .marking = 0,
    .promote_trace = 0,
};

void Xen_GC_GetReady(void) {
  xen_globals->gc_heap = &__gc_heap;
}

struct __GC_Header* Xen_GC_New(Xen_size_t size,
                               void (*fn_trace)(struct __GC_Header*),
                               void (*fn_destroy)(struct __GC_Header*)) {
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

  xen_globals->gc_heap->young_bytes += size;
  xen_globals->gc_heap->total_bytes += size;
  xen_globals->gc_heap->pressure += size;

  if (xen_globals->gc_heap->started) {
    if (xen_globals->gc_heap->pressure >
        xen_globals->gc_heap->minor_threshold) {
      if (xen_globals->gc_heap->major_pressure >
              xen_globals->gc_heap->major_threshold ||
          xen_globals->gc_heap->minor_collections % XEN_GC_MAJOR_EVERY_MINORS ==
              0) {
        Xen_GC_MajorCollect();
      } else {
        Xen_GC_MinorCollect();
      }
    }
  }
  h->trace = fn_trace;
  h->destroy = fn_destroy;
  Xen_GC_Pop_Root();
  return h;
}

void Xen_GC_MinorCollect(void) {
  xen_globals->gc_heap->minor_collections++;
  Xen_GC_Reset_Young();
  Xen_GC_Mark_Young();
  Xen_GC_Sweep_Young();
  xen_globals->gc_heap->pressure = 0;
}

void Xen_GC_MajorCollect(void) {
  xen_globals->gc_heap->major_collections++;
  xen_globals->gc_heap->minor_collections++;
  Xen_GC_Reset_All();
  Xen_GC_Mark();
  Xen_GC_Sweep_Young_Major();
  Xen_GC_Sweep_Old();
  xen_globals->gc_heap->major_pressure = 0;
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

void Xen_GC_Promote_toOld(struct __GC_Header* h) {
  assert(h->generation == GC_YOUNG);
  if (h->prev) {
    h->prev->next = h->next;
  }
  if (h->next) {
    h->next->prev = h->prev;
  }

  if (xen_globals->gc_heap->young == h) {
    xen_globals->gc_heap->young = h->next;
  }

  xen_globals->gc_heap->young_bytes -= h->size;
  xen_globals->gc_heap->old_bytes += h->size;
  xen_globals->gc_heap->major_pressure += h->size;

  h->generation = GC_OLD;
  h->color = GC_WHITE;
  h->age = 0;
  h->rs_handles = NULL;
  h->rs_count = 0;

  h->prev = NULL;
  h->next = xen_globals->gc_heap->old;
  if (xen_globals->gc_heap->old)
    xen_globals->gc_heap->old->prev = h;
  xen_globals->gc_heap->old = h;
}

void Xen_GC_Promote_Trace(struct __GC_Header* h) {
  xen_globals->gc_heap->promote_trace = 1;
  Xen_GC_Trace(h);
  xen_globals->gc_heap->promote_trace = 0;
}

void Xen_GC_Trace_GCHeader(struct __GC_Handle* handle) {
  assert(handle != NULL && handle->ptr != NULL);
  Xen_GCHeader* child = handle->ptr;
  if (xen_globals->gc_heap->promote_trace) {
    Xen_GCHeader* parent = handle->owner;
    if (parent->generation == GC_OLD && child->generation == GC_YOUNG &&
        !(handle->flags & GC_HANDLE_RS)) {
      handle->flags |= GC_HANDLE_RS;
      handle->rs_next = child->rs_handles;
      child->rs_handles = handle;
      child->rs_count++;
    }
    return;
  }
  if (child->color == GC_WHITE) {
    Xen_GC_Push_Gray(child);
  }
}

void Xen_GC_Trace(struct __GC_Header* inst) {
  if (inst->trace) {
    inst->trace(inst);
  }
}

void Xen_GC_Reset_Young(void) {
  struct __GC_Header* curr = xen_globals->gc_heap->young;
  while (curr) {
    curr->color = GC_WHITE;
    curr = curr->next;
  }
}
void Xen_GC_Reset_Old(void) {
  struct __GC_Header* curr = xen_globals->gc_heap->old;
  while (curr) {
    curr->color = GC_WHITE;
    curr = curr->next;
  }
}

void Xen_GC_Reset_All(void) {
  Xen_GC_Reset_Young();
  Xen_GC_Reset_Old();
}

void Xen_GC_Mark(void) {
  xen_globals->gc_heap->marking = 1;
  for (Xen_size_t i = 0; i < xen_globals->gc_heap->roots_count; i++) {
    struct __GC_Header* root = xen_globals->gc_heap->roots[i];
    if (root->color == GC_WHITE) {
      Xen_GC_Push_Gray(xen_globals->gc_heap->roots[i]);
    }
  }
  while (xen_globals->gc_heap->gray_stack_count > 0) {
    struct __GC_Header* inst = Xen_GC_Pop_Gray();
    Xen_GC_Trace(inst);
    inst->color = GC_BLACK;
  }
  xen_globals->gc_heap->marking = 0;
}

void Xen_GC_Mark_Young(void) {
  xen_globals->gc_heap->marking = 1;
  for (Xen_size_t i = 0; i < xen_globals->gc_heap->roots_count; i++) {
    struct __GC_Header* root = xen_globals->gc_heap->roots[i];
    if (root->generation == GC_YOUNG && root->color == GC_WHITE) {
      Xen_GC_Push_Gray(root);
    }
  }
  Xen_GCHeader* curr = xen_globals->gc_heap->young;
  while (curr) {
    if (curr->rs_handles && curr->color == GC_WHITE) {
      Xen_GC_Push_Gray(curr);
    }
    curr = curr->next;
  }
  while (xen_globals->gc_heap->gray_stack_count > 0) {
    struct __GC_Header* inst = Xen_GC_Pop_Gray();
    if (inst->generation == GC_YOUNG) {
      Xen_GC_Trace(inst);
      inst->color = GC_BLACK;
    }
  }
  xen_globals->gc_heap->marking = 0;
}

void Xen_GC_Sweep_Young(void) {
  struct __GC_Header* curr = xen_globals->gc_heap->young;

  while (curr) {
    struct __GC_Header* next = curr->next;

    if (curr->color == GC_WHITE) {
      curr->released = 1;
      if (curr->prev) {
        curr->prev->next = curr->next;
      }
      if (curr->next) {
        curr->next->prev = curr->prev;
      }

      if (xen_globals->gc_heap->young == curr) {
        xen_globals->gc_heap->young = curr->next;
      }

      xen_globals->gc_heap->young_bytes -= curr->size;
      xen_globals->gc_heap->total_bytes -= curr->size;

      curr->prev = NULL;
      curr->next = NULL;
      curr->destroy(curr);

    } else {
      curr->color = GC_WHITE;
      curr->age++;
      if (curr->age >= XEN_GC_PROMOTION_AGE) {
        Xen_GC_Promote_toOld(curr);
        Xen_GC_Promote_Trace(curr);
        Xen_GCHandle* h = curr->rs_handles;
        while (h) {
          h->flags &= ~GC_HANDLE_RS;
          h = h->rs_next;
        }
        curr->rs_handles = NULL;
        curr->rs_count = 0;
      }
    }

    curr = next;
  }
}

void Xen_GC_Sweep_Young_Major(void) {
  struct __GC_Header* curr = xen_globals->gc_heap->young;

  while (curr) {
    struct __GC_Header* next = curr->next;

    if (curr->color == GC_WHITE) {
      curr->released = 1;
      if (curr->prev) {
        curr->prev->next = curr->next;
      }
      if (curr->next) {
        curr->next->prev = curr->prev;
      }

      if (xen_globals->gc_heap->young == curr) {
        xen_globals->gc_heap->young = curr->next;
      }

      xen_globals->gc_heap->young_bytes -= curr->size;
      xen_globals->gc_heap->total_bytes -= curr->size;

      curr->prev = NULL;
      curr->next = NULL;
      curr->destroy(curr);

    } else {
      curr->age++;
    }

    curr = next;
  }
}

void Xen_GC_Sweep_Old(void) {
  struct __GC_Header* curr = xen_globals->gc_heap->old;

  while (curr) {
    struct __GC_Header* next = curr->next;

    if (curr->color == GC_WHITE) {
      curr->released = 1;
      if (curr->prev) {
        curr->prev->next = curr->next;
      }
      if (curr->next) {
        curr->next->prev = curr->prev;
      }

      if (xen_globals->gc_heap->old == curr) {
        xen_globals->gc_heap->old = curr->next;
      }

      xen_globals->gc_heap->old_bytes -= curr->size;
      xen_globals->gc_heap->total_bytes -= curr->size;

      curr->prev = NULL;
      curr->next = NULL;
      curr->destroy(curr);

    } else {
      curr->color = GC_WHITE;
    }

    curr = next;
  }
}

void Xen_GC_Shutdown(void) {
  Xen_GC_MajorCollect();
  assert(xen_globals->gc_heap->young == NULL);
  assert(xen_globals->gc_heap->old == NULL);
  assert(xen_globals->gc_heap->pressure == 0);
  assert(xen_globals->gc_heap->total_bytes == 0);
}

void Xen_GC_Write_Field(struct __GC_Header* parent, struct __GC_Handle** handle,
                        struct __GC_Header* child) {
  struct __GC_Header* old_child = (*handle)->ptr;
  (*handle)->ptr = child;

  if (old_child && ((*handle)->flags & GC_HANDLE_RS) == 1) {
    (*handle)->flags &= ~GC_HANDLE_RS;
  }

  if (!parent || !child)
    return;

  if (xen_globals->gc_heap->marking) {
    if (parent->color == GC_BLACK && child->color == GC_WHITE) {
      Xen_GC_Push_Gray(child);
    }
  }

  if (parent->generation == GC_OLD && child->generation == GC_YOUNG) {
    if (!((*handle)->flags & GC_HANDLE_RS)) {
      (*handle)->flags |= GC_HANDLE_RS;
      (*handle)->rs_next = child->rs_handles;
      child->rs_handles = *handle;
      child->rs_count++;
    }
  }
}

void Xen_GC_Start(void) {
  xen_globals->gc_heap->started = 1;
}

void Xen_GC_Stop(void) {
  xen_globals->gc_heap->started = 0;
}
