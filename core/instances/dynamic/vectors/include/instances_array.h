#ifndef __INSTANCE_ARRAY_H__
#define __INSTANCE_ARRAY_H__

#include <stdbool.h>
#include <stddef.h>

#include "instance.h"

struct __Instances_Array {
  struct __Instance **__inst;
  size_t __size;
  size_t __capacity;
};

struct __Instances_Array *__instances_array_new();
bool __instances_array_push(struct __Instances_Array *, struct __Instance *);
void __instances_array_free(struct __Instances_Array *);

#endif
