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

static int __expose_set_handle(const char* name, Xen_INSTANCE* inst) {
  if (!Xen_Map_Push_Pair_Str(vm->global_props,
                             (Xen_Map_Pair_Str){name, inst})) {
    return 0;
  }
  return 1;
}

static Xen_INSTANCE* __expose_get_handle(const char* name) {
  Xen_INSTANCE* expose = Xen_Map_Get_Str(vm->global_props, name);
  if (!expose) {
    return NULL;
  }
  return expose;
}

static int __args_set_handle(const char* name, Xen_INSTANCE* inst) {
  (void)name;
  (void)inst;
  return 1;
}

static Xen_INSTANCE* __args_get_handle(const char* name) {
  (void)name;
  return Xen_ADD_REF(vm->root_context->ctx_args);
}

static struct Xen_RegisterStream streams[] = {
    {"__expose", true, __expose_set_handle, __expose_get_handle},
    {"__expose_", false, __expose_set_handle, __expose_get_handle},
    {"__args", true, __args_set_handle, __args_get_handle},
    {NULL, false, NULL, NULL},
};

int xen_register_prop_set(const char* name, struct __Instance* inst,
                          ctx_id_t id) {
  if (!name || !inst) {
    return 1;
  }
  for (struct Xen_RegisterStream* f = streams; f->prefix; f++) {
    if ((f->exact_match && strcmp(name, f->prefix) == 0) ||
        (!f->exact_match && strncmp(name, f->prefix, strlen(f->prefix)) == 0)) {
      if (f->set_handle) {
        return f->set_handle(name, inst);
      }
      break;
    }
  }
  Xen_INSTANCE* self = ((RunContext_ptr)Xen_VM_Current_Ctx())->ctx_self;
  if (Xen_Nil_NEval(self) && VM_CHECK_ID(id)) {
    if (XEN_INSTANCE_GET_FLAG(self, XEN_INSTANCE_FLAG_MAPPED)) {
      if (!Xen_Map_Push_Pair_Str(((Xen_INSTANCE_MAPPED*)self)->__map,
                                 (Xen_Map_Pair_Str){name, inst})) {
        return 0;
      }
      return 1;
    }
    return 0;
  }
  if (!Xen_Map_Push_Pair_Str(vm->global_props,
                             (Xen_Map_Pair_Str){name, inst})) {
    return 0;
  }
  return 1;
}

Xen_INSTANCE* xen_register_prop_get(const char* name, ctx_id_t id) {
  if (!name) {
    return NULL;
  }
  for (struct Xen_RegisterStream* f = streams; f->prefix; f++) {
    if ((f->exact_match && strcmp(name, f->prefix) == 0) ||
        (!f->exact_match && strncmp(name, f->prefix, strlen(f->prefix)) == 0)) {
      if (f->get_handle) {
        return f->get_handle(name);
      }
      break;
    }
  }
  Xen_INSTANCE* self = ((RunContext_ptr)Xen_VM_Current_Ctx())->ctx_self;
  if (Xen_Nil_NEval(self) && VM_CHECK_ID(id)) {
    if (XEN_INSTANCE_GET_FLAG(self, XEN_INSTANCE_FLAG_MAPPED)) {
      Xen_INSTANCE* prop =
          Xen_Map_Get_Str(((Xen_INSTANCE_MAPPED*)self)->__map, name);
      if (!prop) {
        goto IMPL;
      }
      return prop;
    }
  IMPL:
    if_nil_neval(self->__impl->__props) {
      Xen_INSTANCE* impl_prop = Xen_Map_Get_Str(self->__impl->__props, name);
      if (!impl_prop) {
        return NULL;
      }
      return impl_prop;
    }
    return NULL;
  }
  Xen_INSTANCE* root_prop = Xen_Map_Get_Str(vm->global_props, name);
  if (!root_prop) {
    return NULL;
  };
  return root_prop;
}
