#ifndef __XEN_VECTOR_ITERATOR_INSTANCE_H__
#define __XEN_VECTOR_ITERATOR_INSTANCE_H__

#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Vector_Iterator_Instance {
  Xen_INSTANCE_HEAD;
  Xen_Instance* vector;
  Xen_ssize_t index;
};

typedef struct Xen_Vector_Iterator_Instance Xen_Vector_Iterator;

#endif
