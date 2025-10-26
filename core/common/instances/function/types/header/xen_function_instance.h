#ifndef __XEN_FUNCTION_INSTANCE_H__
#define __XEN_FUNCTION_INSTANCE_H__

#include "callable.h"
#include "instance.h"

struct Xen_Function_Instance {
  Xen_INSTANCE_HEAD;
  CALLABLE_ptr fun_callable;
  Xen_Instance* closure;
};

typedef struct Xen_Function_Instance Xen_Function;
typedef Xen_Function* Xen_Function_ptr;

#endif
