#include <stdlib.h>

#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "instances_map.h"
#include "xen_nil.h"

struct __Implement *__implement_new(char *impl_name) {
  struct __Implement *impl = (struct __Implement *)__instance_new(&Xen_Basic, nil, 0);
  if_nil_eval(impl) { return NULL; }
  impl->__impl_name = strdup(impl_name);
  if (!impl->__impl_name) {
    free(impl);
    return NULL;
  }
  impl->__props = __instances_map_new(INSTANCES_MAP_DEFAULT_CAPACITY);
  if (!impl->__props) {
    Xen_DEL_REF(impl);
    return NULL;
  }
  impl->__inst_size = sizeof(struct __Instance);
  impl->__alloc = NULL;
  impl->__destroy = NULL;
  impl->__callable = NULL;
  impl->__hash = NULL;
  return impl;
}
