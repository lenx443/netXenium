#ifndef __GC_HEADER_H__
#define __GC_HEADER_H__

#define XEN_GC_HEAD                                                            \
  Xen_uint8_t color;                                                           \
  Xen_uint8_t generation;                                                      \
  Xen_uint16_t age;                                                            \
  Xen_size_t size;                                                             \
  void (*trace)(struct __GC_Header*);                                          \
  void (*destroy)(struct __GC_Header**);                                       \
  struct __GC_Header* next;                                                    \
  struct __GC_Header* prev;

struct __GC_Header;

typedef struct __GC_Header Xen_GCHeader;

#endif
