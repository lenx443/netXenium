#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include "callable.h"
#include <stddef.h>

struct __Instance {
  uint16_t __type_index;
  size_t __value_size;
  void *__value_ptr;
  struct __Instance **__elements_table;
  CALLABLE_ptr __alloc;
  CALLABLE_ptr __destroy;
  CALLABLE_ptr __as_string;
};

struct __Instance *__instance_new();
void __instance_free(struct __Instance *);

#define __XENIUM_INSTANCE struct __Instance *__instance_ptr;

#endif
