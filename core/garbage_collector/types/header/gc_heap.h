#ifndef __GC_HEAP_H__
#define __GC_HEAP_H__

#include "gc_header.h"
#include "xen_typedefs.h"

struct __GC_Heap {
  struct __GC_Header* young;
  struct __GC_Header* old;
  Xen_size_t total_bytes;
  Xen_size_t threshold;
  Xen_size_t pressure;
};

#endif
