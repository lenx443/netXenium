#ifndef __XEN_TUPLE_INSTANCE_H__
#define __XEN_TUPLE_INSTANCE_H__

#include "instance.h"

struct Xen_Tuple_Instance {
  Xen_INSTANCE_HEAD;
  Xen_Instance **instances;
};

typedef struct Xen_Tuple_Instance Xen_Tuple;

#endif
