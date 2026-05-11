#ifndef __COROUTINE_INSTANCE_H__
#define __COROUTINE_INSTANCE_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_except_instance.h"
#include "xen_function_instance.h"
#include "xen_typedefs.h"

typedef struct Xen_Coroutine_Instance {
  Xen_INSTANCE_HEAD
  int type;
  Xen_GCHandle* context;
  struct {
    Xen_Native_Func_Async func_async;
    Xen_GCHandle* self;
    Xen_GCHandle* args;
    Xen_GCHandle* kwargs;
    void* data;
  };
  Xen_GCHandle* awaited;
  Xen_GCHandle* awaiter;
  Xen_GCHandle* result;
  struct Xen_Except_Status except;
  Xen_uint8_t status;
} Xen_Coroutine;

#endif
