#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "instances_map.h"
#include "run_ctx.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"

static int module_alloc(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Module *module = (Xen_Module *)self;
  module->mod_map = NULL;
  module->mod_context = nil;
  return 1;
}

static int module_destroy(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Module *module = (Xen_Module *)self;
  if (module->mod_map) __instances_map_free(module->mod_map);
  if_nil_neval(module->mod_context) Xen_DEL_REF(module->mod_context);
  return 1;
}

static int module_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Instance *string = Xen_String_From_CString("<Module>");
  if (!string) { return 0; }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

struct __Implement Xen_Module_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Module",
    .__inst_size = sizeof(struct Xen_Module_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = module_alloc,
    .__destroy = module_destroy,
    .__string = module_string,
    .__raw = module_string,
    .__callable = NULL,
    .__hash = NULL,
};
