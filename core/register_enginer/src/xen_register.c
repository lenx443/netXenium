#include <stdbool.h>
#include <string.h>

#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "vm_def.h"
#include "xen_register.h"
#include "xen_register_flow.h"

static Xen_INSTANCE *__expose_handle(const char *name) {
  Xen_INSTANCE *expose = __instances_map_get(vm->global_props, name);
  if (!expose) { return NULL; }
  Xen_ADD_REF(expose);
  return expose;
}

static struct Xen_RegisterFlow flows[] = {
    {"__expose", true, __expose_handle},
    {"__expose_", false, __expose_handle},
    {NULL, false, NULL},
};

Xen_INSTANCE *xen_register_prop_get(const char *name, ctx_id_t id) {
  if (!name || !VM_CHECK_ID(id)) { return NULL; }
  for (struct Xen_RegisterFlow *f = flows; f->prefix; f++) {
    if ((f->exact_match && strcmp(name, f->prefix) == 0) ||
        (!f->exact_match && strncmp(name, f->prefix, strlen(f->prefix)) == 0)) {
      if (f->get_handle) { return f->get_handle(name); }
      return NULL;
    }
  }
  Xen_INSTANCE *self = vm_current_ctx()->ctx_self;
  if (XEN_INSTANCE_GET_FLAG(self, XEN_INSTANCE_FLAG_MAPPED)) {
    Xen_INSTANCE *prop = __instances_map_get(((Xen_INSTANCE_MAPPED *)self)->__map, name);
    if (!prop) {
      Xen_INSTANCE *impl_prop = __instances_map_get(self->__impl->__props, name);
      if (!impl_prop) { return NULL; }
      Xen_ADD_REF(impl_prop);
      return impl_prop;
    }
    Xen_ADD_REF(prop);
    return prop;
  }
  Xen_INSTANCE *impl_prop = __instances_map_get(self->__impl->__props, name);
  if (!impl_prop) { return NULL; }
  Xen_ADD_REF(impl_prop);
  return impl_prop;
}
