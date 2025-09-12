#include "instance.h"
#include "instances_map.h"
#include "vm_def.h"
#include "xen_module.h"
#include "xen_module_load.h"
#include "xen_module_types.h"
#include "xen_modules_def.h"
#include "xen_nil.h"

int Xen_Module_Load_Startup() {
  for (int i = 0; xen_startup_modules[i] != NULL; i++) {
    struct Xen_Module_Def *mod = xen_startup_modules[i];
    Xen_Instance *mod_inst = Xen_Module_From_Def(*mod);
    if_nil_eval(mod_inst) { return 0; }
    if (!__instances_map_add(vm->root_context->ctx_instances, mod->mod_name, mod_inst)) {
      Xen_DEL_REF(mod_inst);
      return 0;
    }
    Xen_DEL_REF(mod_inst);
  }
  return 1;
}
