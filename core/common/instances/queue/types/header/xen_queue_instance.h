#ifndef __XEN_QUEUE_INSTANCE_H__
#define __XEN_QUEUE_INSTANCE_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_typedefs.h"

typedef struct Xen_Queue_Instance {
  Xen_INSTANCE_HEAD
  Xen_GCHandle** buf;
  Xen_size_t head;
  Xen_size_t tail;
  Xen_size_t capacity;
  Xen_size_t size;
} Xen_Queue;

#endif
