#ifndef __COROUTINE_INSTANCE_H__
#define __COROUTINE_INSTANCE_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_except_instance.h"
#include "xen_typedefs.h"

typedef struct Xen_Coroutine_Instance {
  Xen_INSTANCE_HEAD
  Xen_GCHandle* context;
  Xen_GCHandle* caller;
  Xen_GCHandle* result;
  struct Xen_Except_Status except;
  Xen_uint8_t status;
} Xen_Coroutine;

#endif
