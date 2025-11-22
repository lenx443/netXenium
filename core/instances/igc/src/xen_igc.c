#include "xen_igc.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_gc.h"
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

struct __IGC_Roots* __igc_roots_list = NULL;

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

static struct __IGC_Roots* __igc_roots_new() {
  struct __IGC_Roots* roots = (struct __IGC_Roots*)Xen_GC_New(
      sizeof(struct __IGC_Roots), __igc_roots_trace, __igc_roots_destroy);
  roots->roots =
      Xen_Alloc(__XEN_IGC_INITIAL_ROOTS_CAP__ * sizeof(Xen_GCHeader*));
  roots->count = 0;
  roots->cap = __XEN_IGC_INITIAL_ROOTS_CAP__;
  return roots;
}

static void __igc_roots_ensure_capacity() {
  if (__igc_roots_list->count < __igc_roots_list->cap) {
    return;
  }
  Xen_size_t cap = __igc_roots_list->cap ? __igc_roots_list->cap * 2
                                         : __XEN_IGC_INITIAL_ROOTS_CAP__;
  __igc_roots_list->roots =
      Xen_Realloc(__igc_roots_list->roots, cap * sizeof(Xen_GCHeader*));
  __igc_roots_list->cap = cap;
}

void Xen_IGC_Init() {
  __igc_roots_list = __igc_roots_new();
  Xen_GC_Push_Root((Xen_GCHeader*)__igc_roots_list);
}

void Xen_IGC_Finish() {
  Xen_GC_Pop_Root();
}

void Xen_IGC_Push(Xen_Instance* inst) {
  __igc_roots_ensure_capacity();
  __igc_roots_list->roots[__igc_roots_list->count++] = (Xen_GCHeader*)inst;
}

void Xen_IGC_Pop() {
  assert(__igc_roots_list->count > 0);
  __igc_roots_list->count--;
}
