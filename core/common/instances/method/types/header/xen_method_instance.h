#ifndef __XEN_METHOD_INSTANCE_H__
#define __XEN_METHOD_INSTANCE_H__

#include "instance.h"

struct Xen_Method_Instance {
  Xen_INSTANCE_HEAD;
  Xen_GCHandle* function;
  Xen_GCHandle* self;
};

typedef struct Xen_Method_Instance Xen_Method;

#endif
