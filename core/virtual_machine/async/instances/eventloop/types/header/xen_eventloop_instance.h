#ifndef __XEN_EVENTLOOP_INSTANCE_H__
#define __XEN_EVENTLOOP_INSTANCE_H__

#include "gc_header.h"
#include "instance.h"

typedef struct Xen_EventLoop_Instance {
  Xen_INSTANCE_HEAD
  Xen_GCHandle* tasks;
  Xen_GCHandle* resumed;
} Xen_EventLoop;

#endif
