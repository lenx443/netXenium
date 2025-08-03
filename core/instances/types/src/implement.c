#include <stdlib.h>

#include "callable.h"
#include "implement.h"

struct __Implement *__implement_new(char *impl_name) {
  struct __Implement *impl = malloc(sizeof(struct __Implement));
  if (!impl) { return NULL; }
  impl->__type_index = 0;
  impl->__impl_name = strdup(impl_name);
  impl->__alloc = NULL;
  impl->__destroy = NULL;
  impl->__as_string = NULL;
  return impl;
}

void __implement_free(struct __Implement *impl) {
  if (!impl) return;
  free(impl->__impl_name);
  callable_free(impl->__alloc);
  callable_free(impl->__destroy);
  callable_free(impl->__as_string);
  free(impl);
}
