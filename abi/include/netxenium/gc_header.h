#ifndef __GC_HEADER_H__
#define __GC_HEADER_H__

#include "xen_typedefs.h"

#define GC_WHITE 1
#define GC_GRAY 2
#define GC_BLACK 3

#define GC_YOUNG 1
#define GC_OLD 2

#define XEN_GC_HEAD                                                            \
  Xen_uint8_t color;                                                           \
  Xen_uint8_t generation;                                                      \
  Xen_uint8_t released;                                                        \
  Xen_uint16_t age;                                                            \
  Xen_size_t size;                                                             \
  void (*trace)(struct __GC_Header*);                                          \
  void (*destroy)(struct __GC_Header*);                                        \
  struct __GC_Header* next;                                                    \
  struct __GC_Header* prev;                                                    \
  struct __GC_Handle* rs_handles;                                              \
  Xen_size_t rs_count;

struct __GC_Header;

struct __GC_Handle {
  Xen_uint8_t flags;
  struct __GC_Header* ptr;
  struct __GC_Header* owner;
  struct __GC_Handle* rs_next;
};

typedef struct __GC_Header Xen_GCHeader;
typedef struct __GC_Handle Xen_GCHandle;

struct __GC_Handle* Xen_GCHandle_New(struct __GC_Header*);
struct __GC_Handle* Xen_GCHandle_New_From(struct __GC_Header*, Xen_GCHeader*);
void Xen_GCHandle_Free(struct __GC_Handle*);

#endif
