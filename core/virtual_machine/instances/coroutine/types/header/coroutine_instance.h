#ifndef __COROUTINE_INSTANCE_H__
#define __COROUTINE_INSTANCE_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_typedefs.h"

typedef struct Xen_Coroutine_Instance {
  Xen_INSTANCE_HEAD
  Xen_GCHandle* context;
  Xen_uint8_t status;
} Xen_Coroutine;

#endif
