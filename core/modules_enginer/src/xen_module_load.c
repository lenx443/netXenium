#include "xen_module_load.h"
#include "instance.h"
#include "vm_def.h"
#include "xen_map.h"
#include "xen_module.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_modules_def.h"
#include <string.h>

static int core_success = 0;

static int load_module_core(struct Xen_Module_Def* mod) {
  if (core_success)
    return 0;
  core_success = 1;
  Xen_Instance* mod_inst = Xen_Module_From_Def(*mod);
  if (!mod_inst) {
    return 0;
  }
  if (!Xen_Map_Push_Map(vm->root_context->ctx_instances,
                        ((Xen_Module*)mod_inst)->mod_map)) {
    Xen_DEL_REF(mod_inst);
    return 0;
  }
  Xen_DEL_REF(mod_inst);
  return 1;
}

int Xen_Module_Load_Startup() {
  for (int i = 0; xen_startup_modules[i] != NULL; i++) {
    struct Xen_Module_Def* mod = xen_startup_modules[i];
    if (strcmp(mod->mod_name, "core") == 0) {
      if (!load_module_core(mod)) {
        return 0;
      }
    } else {
      Xen_Instance* mod_inst = Xen_Module_From_Def(*mod);
      if (!mod_inst) {
        return 0;
      }
      if (!Xen_Map_Push_Pair_Str(vm->root_context->ctx_instances,
                                 (Xen_Map_Pair_Str){mod->mod_name, mod_inst})) {
        Xen_DEL_REF(mod_inst);
        return 0;
      }
      Xen_DEL_REF(mod_inst);
    }
  }
  return 1;
}
