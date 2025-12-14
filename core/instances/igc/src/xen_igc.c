#include "xen_igc.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_typedefs.h"
#include <assert.h>

#ifndef __XEN_IGC_INITIAL_ROOTS_CAP__
#define __XEN_IGC_INITIAL_ROOTS_CAP__ 64
#endif

struct __IGC_Roots {
  Xen_GCHeader gc;
  Xen_GCHeader** roots;
  Xen_size_t count;
  Xen_size_t cap;
};

static struct __IGC_Roots* __igc_roots_list = NULL;

static void __igc_roots_trace(Xen_GCHeader* h) {
  struct __IGC_Roots* roots = (struct __IGC_Roots*)h;
  for (Xen_size_t i = 0; i < roots->count; i++) {
    Xen_GC_Trace_GCHeader(roots->roots[i]);
  }
}

static void __igc_roots_destroy(Xen_GCHeader** h) {
  struct __IGC_Roots* roots = (struct __IGC_Roots*)*h;
  if (roots->roots) {
    Xen_Dealloc(roots->roots);
  }
  Xen_Dealloc(*h);
}

static struct __IGC_Roots* __igc_roots_new(void) {
  struct __IGC_Roots* roots = (struct __IGC_Roots*)Xen_GC_New(
      sizeof(struct __IGC_Roots), __igc_roots_trace, __igc_roots_destroy);
  roots->roots =
      Xen_Alloc(__XEN_IGC_INITIAL_ROOTS_CAP__ * sizeof(Xen_GCHeader*));
  roots->count = 0;
  roots->cap = __XEN_IGC_INITIAL_ROOTS_CAP__;
  return roots;
}

static void __igc_roots_ensure_capacity(struct __IGC_Roots* roots) {
  if (roots->count < roots->cap) {
    return;
  }
  Xen_size_t cap = roots->cap ? roots->cap * 2 : __XEN_IGC_INITIAL_ROOTS_CAP__;
  roots->roots = Xen_Realloc(roots->roots, cap * sizeof(Xen_GCHeader*));
  roots->cap = cap;
}

static void __igc_roots_push(struct __IGC_Roots* roots, Xen_GCHeader* h) {
  __igc_roots_ensure_capacity(roots);
  roots->roots[roots->count++] = h;
}

static void __igc_roots_pop(struct __IGC_Roots* roots) {
  assert(roots->count > 0);
  roots->count--;
}

void Xen_IGC_Init(void) {
  __igc_roots_list = __igc_roots_new();
  Xen_GC_Push_Root((Xen_GCHeader*)__igc_roots_list);
  xen_globals->igc_roots = &__igc_roots_list;
}

void Xen_IGC_Finish(void) {
  Xen_GC_Pop_Root();
}

void Xen_IGC_Push(Xen_Instance* inst) {
  __igc_roots_push((*xen_globals->igc_roots), (Xen_GCHeader*)inst);
}

void Xen_IGC_Pop(void) {
  __igc_roots_pop((*xen_globals->igc_roots));
}

struct __IGC_Roots* Xen_IGC_Fork_New(void) {
  struct __IGC_Roots* fork = __igc_roots_new();
  __igc_roots_push((*xen_globals->igc_roots), (Xen_GCHeader*)fork);
  return fork;
}

void Xen_IGC_Fork_Push(struct __IGC_Roots* f, Xen_Instance* inst) {
  __igc_roots_push(f, (Xen_GCHeader*)inst);
}

void Xen_IGC_Fork_Pop(struct __IGC_Roots* f) {
  __igc_roots_pop(f);
}
