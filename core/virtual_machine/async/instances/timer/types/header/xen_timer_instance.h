#ifndef __XEN_TIMER_INSTANCE_H__
#define __XEN_TIMER_INSTANCE_H__

#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Timer_Instance {
  Xen_INSTANCE_HEAD
  Xen_GCHandle* coroutine;
  Xen_uint64_t expire;
  Xen_size_t index;
  int cancelled;
};

typedef struct Xen_Timer_Instance Xen_Timer;

#endif
