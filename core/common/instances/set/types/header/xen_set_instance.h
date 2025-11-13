#ifndef __XEN_SET_INSTANCE_H__
#define __XEN_SET_INSTANCE_H__

#include "instance.h"
#include "xen_typedefs.h"

struct __Set_Node {
  Xen_Instance* value;
  Xen_size_t* index;
  struct __Set_Node* next;
};

struct Xen_Set_Instance {
  Xen_INSTANCE_HEAD;
  Xen_Instance* values;
  struct __Set_Node** buckets;
  Xen_size_t capacity;
};

typedef struct Xen_Set_Instance Xen_Set;

#endif
