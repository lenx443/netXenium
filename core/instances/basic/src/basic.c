#include <stdbool.h>
#include <stdlib.h>

#include "basic.h"
#include "call_args.h"
#include "implement.h"
#include "instance.h"

int basic_alloc(struct __Instance *inst, CallArgs *args) { return 1; }

int basic_destroy(struct __Instance *inst, CallArgs *args) {
  struct __Implement *impl = (struct __Implement *)inst;
  if (!impl) return false;
  if (impl->__props) __instances_map_free(impl->__props);
  if (impl->__impl_name) free(impl->__impl_name);
  return 1;
}

int basic_callable(struct __Instance *inst, CallArgs *args) { return 1; }

struct __Implement Xen_Basic = {
    Xen_INSTANCE_SET(0, &Xen_Basic, 0),
    .__impl_name = "Basic",
    .__inst_size = sizeof(struct __Implement),
    .__props = NULL,
    .__alloc = basic_alloc,
    .__destroy = basic_destroy,
    .__callable = basic_callable,
    .__hash = NULL,
};
