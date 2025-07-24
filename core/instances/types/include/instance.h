#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include "callable.h"

struct __Instance {
  uint16_t __type_index;
  void *__value_ptr;
  CALLABLE_ptr __as_string;
};

struct __Instance *__instance_new();
const struct __Instance *__instance_new_const(uint16_t, void *, CALLABLE_ptr);

#define __XENIUM_INSTANCE struct __Instance *__instance_ptr;
#define __XENIUM_CONST const struct __Instance *__instance_ptr;

#endif
