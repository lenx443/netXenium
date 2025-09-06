#ifndef __XEN_MAP_INSTANCE_H__
#define __XEN_MAP_INSTANCE_H__

#include <stddef.h>

#include "instance.h"

struct __Map_Node {
  Xen_Instance *key;
  Xen_Instance *value;
  struct __Map_Node *next;
};

struct Xen_Map_Instance {
  Xen_INSTANCE_HEAD;
  struct __Map_Node **map_buckets;
  size_t map_capacity;
};

typedef struct Xen_Map_Instance Xen_Map;

#endif
