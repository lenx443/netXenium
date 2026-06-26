#ifndef __XEN_EVENTLOOP_INSTANCE_H__
#define __XEN_EVENTLOOP_INSTANCE_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_typedefs.h"

typedef struct Xen_EventLoop_Instance {
  Xen_INSTANCE_HEAD
  Xen_GCHandle* tasks;
  Xen_GCHandle* resumed;
  Xen_GCHandle* timer_heap;
  Xen_GCHandle* cb_interrupt;
  int timer_fd;
  int event_fd;
  Xen_size_t io_refs;
} Xen_EventLoop;

#endif
