#ifndef __GC_HEAP_H__
#define __GC_HEAP_H__

#include "gc_header.h"

struct __GC_Heap {
  struct __GC_Header* young;
  struct __GC_Header* old;
};

#endif
