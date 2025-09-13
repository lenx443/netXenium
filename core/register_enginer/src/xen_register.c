#include <stdbool.h>
#include <string.h>

#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "vm_def.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_register_stream.h"

static int __expose_set_handle(const char *name, Xen_INSTANCE *inst) {
  if (!Xen_Map_Push_Pair_Str(vm->global_props, (Xen_Map_Pair_Str){name, inst})) {
    return 0;
  }
  return 1;
}

static Xen_INSTANCE *__expose_get_handle(const char *name) {
  Xen_INSTANCE *expose = Xen_Map_Get_Str(vm->global_props, name);
  if_nil_eval(expose) { return nil; }
  return expose;
}

static struct Xen_RegisterStream streams[] = {
    {"__expose", true, __expose_set_handle, __expose_get_handle},
    {"__expose_", false, __expose_set_handle, __expose_get_handle},
    {NULL, false, NULL, NULL},
};

int xen_register_prop_set(const char *name, struct __Instance *inst, ctx_id_t id) {
  if (!name || !inst) { return 1; }
  for (struct Xen_RegisterStream *f = streams; f->prefix; f++) {
    if ((f->exact_match && strcmp(name, f->prefix) == 0) ||
        (!f->exact_match && strncmp(name, f->prefix, strlen(f->prefix)) == 0)) {
      if (f->set_handle) { return f->set_handle(name, inst); }
      break;
    }
  }
  Xen_INSTANCE *self = ((RunContext_ptr)vm_current_ctx())->ctx_self;
  if (self && VM_CHECK_ID(id)) {
    if (XEN_INSTANCE_GET_FLAG(self, XEN_INSTANCE_FLAG_MAPPED)) {
      if (!Xen_Map_Push_Pair_Str(((Xen_INSTANCE_MAPPED *)self)->__map,
                                 (Xen_Map_Pair_Str){name, inst})) {
        return 0;
      }
      return 1;
    }
    return 0;
  }
  if (!Xen_Map_Push_Pair_Str(vm->global_props, (Xen_Map_Pair_Str){name, inst})) {
    return 0;
  }
  return 1;
}

Xen_INSTANCE *xen_register_prop_get(const char *name, ctx_id_t id) {
  if (!name) { return nil; }
  for (struct Xen_RegisterStream *f = streams; f->prefix; f++) {
    if ((f->exact_match && strcmp(name, f->prefix) == 0) ||
        (!f->exact_match && strncmp(name, f->prefix, strlen(f->prefix)) == 0)) {
      if (f->get_handle) { return f->get_handle(name); }
      break;
    }
  }
  Xen_INSTANCE *self = ((RunContext_ptr)vm_current_ctx())->ctx_self;
  if (self && VM_CHECK_ID(id)) {
    if (XEN_INSTANCE_GET_FLAG(self, XEN_INSTANCE_FLAG_MAPPED)) {
      Xen_INSTANCE *prop = Xen_Map_Get_Str(((Xen_INSTANCE_MAPPED *)self)->__map, name);
      if_nil_eval(prop) { goto IMPL; }
      return prop;
    }
  IMPL:
    if_nil_neval(self->__impl->__props) {
      Xen_INSTANCE *impl_prop = Xen_Map_Get_Str(self->__impl->__props, name);
      if_nil_eval(impl_prop) { return nil; }
      Xen_ADD_REF(impl_prop);
      return impl_prop;
    }
    return nil;
  }
  Xen_INSTANCE *root_prop = Xen_Map_Get_Str(vm->global_props, name);
  if_nil_eval(root_prop) { return nil; };
  Xen_ADD_REF(root_prop);
  return root_prop;
}
