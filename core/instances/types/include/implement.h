#ifndef __IMPLEMENT_H__
#define __IMPLEMENT_H__

#include "callable.h"
#include <stddef.h>

struct __Instance;

struct __Implement {
  uint16_t __type_index;
  char *__impl_name;
  size_t __inst_size;
  CALLABLE_ptr __alloc;
  CALLABLE_ptr __destroy;
  CALLABLE_ptr __callable;
  CALLABLE_ptr __hash;
};

struct __Implement *__implement_new(char *);
void __implement_free(struct __Implement *);

#endif
