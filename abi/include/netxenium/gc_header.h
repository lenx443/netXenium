#ifndef __GC_HEADER_H__
#define __GC_HEADER_H__

#define GC_WHITE 1
#define GC_GRAY 2
#define GC_BLACK 3

#define GC_YOUNG 1
#define GC_OLD 2

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
