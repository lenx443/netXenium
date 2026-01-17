#include "gc_header.h"
#include "xen_alloc.h"

struct __GC_Handle* Xen_GCHandle_New(struct __GC_Header* owner) {
  Xen_GCHandle* handle = Xen_Alloc(sizeof(struct __GC_Handle));
  handle->flags = 0;
  handle->ptr = NULL;
  handle->owner = owner;
  handle->rs_next = NULL;
  return handle;
}

struct __GC_Handle* Xen_GCHandle_New_From(struct __GC_Header* owner,
                                          Xen_GCHeader* h) {
  Xen_GCHandle* handle = Xen_Alloc(sizeof(struct __GC_Handle));
  handle->flags = 0;
  handle->ptr = h;
  handle->owner = owner;
  handle->rs_next = NULL;
  return handle;
}

void Xen_GCHandle_Free(struct __GC_Handle* handle) {
  Xen_Dealloc(handle);
}
