#ifndef __INSTANCE_ARRAY_H__
#define __INSTANCE_ARRAY_H__

#include <stdbool.h>
#include <stddef.h>

#include "instance.h"

struct __Instances_Hash_Node {
  char *name;
  size_t index;
  struct __Instances_Hash_Node *next;
};

struct __Instances_Map {
  struct __Instances_Hash_Node **__buckets;
  size_t __buckets_capacity;
  Xen_INSTANCE **__vec_inst;
  size_t __vec_size;
  size_t __vec_capacity;
};

struct __Instances_Map *__instances_map_new(size_t);
bool __instances_map_add(struct __Instances_Map *, const char *, Xen_INSTANCE *);
Xen_INSTANCE *__instances_map_get(struct __Instances_Map *, const char *);
void __instances_map_free(struct __Instances_Map *);

#endif
