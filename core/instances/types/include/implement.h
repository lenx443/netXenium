#ifndef __IMPLEMENT_H__
#define __IMPLEMENT_H__

#include "callable.h"
#include <stddef.h>

struct __Implement {
  uint16_t __type_index;
  char *__impl_name;
  CALLABLE_ptr __alloc;
  CALLABLE_ptr __destroy;
  CALLABLE_ptr __as_string;
};

struct __Implement *__implement_new(char *);
void __implement_free(struct __Implement *);

#endif
