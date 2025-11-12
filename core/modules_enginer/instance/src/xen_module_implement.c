#include "xen_module_implement.h"
#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_alloc.h"
#include "xen_module_instance.h"
#include "xen_nil.h"
#include "xen_string.h"

static Xen_Instance* module_alloc(ctx_id_t id, Xen_Instance* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Module* module = (Xen_Module*)Xen_Instance_Alloc(&Xen_Module_Implement);
  if (!module) {
    return NULL;
  }
  module->mod_map = nil;
  module->mod_context = nil;
  return (Xen_Instance*)module;
}

static Xen_Instance* module_destroy(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Module* module = (Xen_Module*)self;
  if_nil_neval(module->mod_map) Xen_DEL_REF(module->mod_map);
  if_nil_neval(module->mod_context) Xen_DEL_REF(module->mod_context);
  return nil;
}

static Xen_Instance* module_string(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<Module>");
  if (!string) {
    return NULL;
  }
  return string;
}

struct __Implement Xen_Module_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Module",
    .__inst_size = sizeof(struct Xen_Module_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = module_alloc,
    .__create = NULL,
    .__destroy = module_destroy,
    .__string = module_string,
    .__raw = module_string,
    .__callable = NULL,
    .__hash = NULL,
};
