#ifndef __IMPLEMENT_H__
#define __IMPLEMENT_H__

#include <stddef.h>

#include "callable.h"
#include "instance.h"
#include "instances_map.h"

struct __Instance;

struct __Implement {
  Xen_INSTANCE_HEAD;
  char *__impl_name;
  size_t __inst_size;
  Xen_Instance_Flag __inst_default_flags;
  struct __Instances_Map *__props;
  Xen_Native_Func __alloc;
  Xen_Native_Func __destroy;
  Xen_Native_Func __callable;
  Xen_Native_Func __hash;
};

struct __Implement *__implement_new(char *);

#endif
