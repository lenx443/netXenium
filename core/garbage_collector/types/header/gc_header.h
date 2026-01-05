#ifndef __GC_HEADER_H__
#define __GC_HEADER_H__

#include "xen_alloc.h"
#include "xen_typedefs.h"

#define GC_WHITE 1
#define GC_GRAY 2
#define GC_BLACK 3

#define GC_YOUNG 1
#define GC_OLD 2

struct __GC_Header {
  Xen_uint8_t color;
  Xen_uint8_t generation;
  Xen_uint8_t released;
  Xen_uint16_t age;
  Xen_size_t size;
  void (*trace)(struct __GC_Header*);
  void (*destroy)(struct __GC_Header*);
  struct __GC_Header* next;
  struct __GC_Header* prev;
};

struct __GC_Handle {
  struct __GC_Header* ptr;
};

typedef struct __GC_Header Xen_GCHeader;
typedef struct __GC_Handle Xen_GCHandle;

static inline struct __GC_Handle* Xen_GCHandle_New(void) {
  Xen_GCHandle* handle = Xen_Alloc(sizeof(struct __GC_Handle));
  handle->ptr = NULL;
  return handle;
}

static inline struct __GC_Handle* Xen_GCHandle_New_From(Xen_GCHeader* h) {
  Xen_GCHandle* handle = Xen_Alloc(sizeof(struct __GC_Handle));
  handle->ptr = h;
  return handle;
}

static inline void Xen_GCHandle_Free(struct __GC_Handle* handle) {
  Xen_Dealloc(handle);
}

#endif
