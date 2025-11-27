#ifndef __XEN_FUNCTION_INSTANCE_H__
#define __XEN_FUNCTION_INSTANCE_H__

#include "callable.h"
#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Function_Instance {
  Xen_INSTANCE_HEAD;
  Xen_uint8_t fun_type;
  CALLABLE_ptr fun_code;
  Xen_Native_Func fun_native;
  Xen_Instance* closure;
  Xen_Instance* args_names;
  Xen_Instance* args_default_values;
  Xen_ssize_t args_requireds;
};

typedef struct Xen_Function_Instance Xen_Function;
typedef Xen_Function* Xen_Function_ptr;

#endif
