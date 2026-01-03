#ifndef __XEN_TUPLE_ITERATOR_INSTANCE_H__
#define __XEN_TUPLE_ITERATOR_INSTANCE_H__

#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Tuple_Iterator_Instance {
  Xen_INSTANCE_HEAD;
  Xen_GCHandle* tuple;
  Xen_ssize_t index;
};

typedef struct Xen_Tuple_Iterator_Instance Xen_Tuple_Iterator;

#endif
