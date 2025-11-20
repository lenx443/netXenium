#ifndef __XEN_GC_H__
#define __XEN_GC_H__

#include "gc_header.h"

void Xen_GC_Push_Root(struct __GC_Header*);
void Xen_GC_Pop_Root();
void Xen_GC_Push_Gray(struct __GC_Header*);
struct __GC_Header* Xen_GC_Pop_Gray();
void Xen_GC_Trace(struct __GC_Header*);
void Xen_GC_Mark();
void Xen_GC_Sweep();

static inline void Xen_GC_Write_Field(struct __GC_Header* parent,
                                      struct __GC_Header** field,
                                      struct __GC_Header* child) {
  *field = child;
  if (parent && parent->color == GC_BLACK && child->color == GC_WHITE) {
    Xen_GC_Push_Gray(child);
  }
}

#endif
