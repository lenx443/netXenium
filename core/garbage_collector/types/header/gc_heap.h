#ifndef __GC_HEAP_H__
#define __GC_HEAP_H__

#include "gc_header.h"
#include "xen_typedefs.h"

#ifndef __XEN_GC_MAX_ROOTS__
#define __XEN_GC_MAX_ROOTS__ 1024
#endif

#ifndef __XEN_GC_MAX_GRAY__
#define __XEN_GC_MAX_GRAY__ 65536
#endif

#ifndef __XEN_GC_MAX_REMEMBERED__
#define __XEN_GC_MAX_REMEMBERED__ 65536
#endif

struct __GC_Edge {
  struct __GC_Header* parent;
  struct __GC_Handle* handle;
};

struct __GC_Heap {
  struct __GC_Header* young;
  struct __GC_Header* old;

  struct __GC_Header* roots[__XEN_GC_MAX_ROOTS__];
  struct __GC_Header* gray_stack[__XEN_GC_MAX_GRAY__];
  struct __GC_Edge remembered_set[__XEN_GC_MAX_REMEMBERED__];

  Xen_size_t roots_count;
  Xen_size_t gray_stack_count;
  Xen_size_t remembered_count;

  Xen_size_t young_bytes;
  Xen_size_t old_bytes;

  Xen_size_t minor_collections;
  Xen_size_t major_collections;

  Xen_size_t minor_threshold;
  Xen_size_t major_threshold;

  Xen_size_t major_pressure;

  Xen_size_t total_bytes;
  Xen_size_t pressure;

  Xen_bool_t started;
  Xen_bool_t marking;
};

#endif
