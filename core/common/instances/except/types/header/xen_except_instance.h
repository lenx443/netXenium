#ifndef __XEN_EXCEPT_INSTANCE_H__
#define __XEN_EXCEPT_INSTANCE_H__

#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Except_Instance {
  Xen_INSTANCE_HEAD;
  Xen_c_string_t type;
  Xen_c_string_t message;
};

typedef struct Xen_Except_Instance Xen_Except;

#endif
