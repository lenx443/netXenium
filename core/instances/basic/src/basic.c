#include <stdlib.h>

#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_register.h"
#include "xen_string.h"

static int basic_alloc(ctx_id_t id, struct __Instance *self, Xen_Instance *args) {
  struct __Implement *impl = (struct __Implement *)self;
  impl->__impl_name = NULL;
  impl->__props = __instances_map_new(INSTANCES_MAP_DEFAULT_CAPACITY);
  if (!impl->__props) {
    Xen_DEL_REF(impl);
    return 0;
  }
  impl->__inst_size = sizeof(struct __Instance);
  impl->__alloc = NULL;
  impl->__destroy = NULL;
  impl->__callable = NULL;
  impl->__hash = NULL;
  return 1;
}

static int basic_destroy(ctx_id_t id, struct __Instance *self, Xen_Instance *args) {
  struct __Implement *impl = (struct __Implement *)self;
  if (!impl) return 0;
  if (impl->__props) __instances_map_free(impl->__props);
  if (impl->__impl_name) free(impl->__impl_name);
  return 1;
}

static int basic_callable(ctx_id_t id, struct __Instance *self, Xen_Instance *args) {
  struct __Implement *impl = (struct __Implement *)self;
  Xen_INSTANCE *result = __instance_new(impl, args, 0);
  xen_register_prop_set("__expose", result, id);
  return 1;
}

static int basic_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Instance *string = Xen_String_From_CString("<Basic>");
  if (!string) { return 0; }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

struct __Implement Xen_Basic = {
    Xen_INSTANCE_SET(0, &Xen_Basic, 0),
    .__impl_name = "Basic",
    .__inst_size = sizeof(struct __Implement),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = basic_alloc,
    .__destroy = basic_destroy,
    .__callable = basic_callable,
    .__hash = NULL,
};
