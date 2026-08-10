#include <string.h>

#include "instance.h"
#include "vm.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_module.h"
#include "xen_module_instance.h"
#include "xen_module_load.h"
#include "xen_module_types.h"
#include "xen_modules_def.h"

static int load_module_core(struct Xen_Module_Def* mod) {
  if (xen_globals->program->mod_core_success)
    return 0;
  xen_globals->program->mod_core_success = 1;
  Xen_Instance* mod_inst = Xen_Module_From_Def(*mod, "builtin://", NULL);
  if (!mod_inst) {
    return 0;
  }
  if (!Xen_Map_Push_Map(
          (Xen_Instance*)Xen_VM()->globals_instances->ptr,
          (Xen_Instance*)((Xen_Module*)mod_inst)->__map->ptr)) {
    return 0;
  }
  return 1;
}

int Xen_Module_Load_Startup(void) {
  for (int i = 0; xen_startup_modules[i] != NULL; i++) {
    struct Xen_Module_Def* mod = xen_startup_modules[i];
    if (strcmp(mod->mod_name, "core") == 0) {
      if (!load_module_core(mod)) {
        return 0;
      }
    } else {
      Xen_Instance* mod_inst = Xen_Module_From_Def(*mod, "builtin://", NULL);
      if (!mod_inst) {
        return 0;
      }
      if (!Xen_Map_Push_Pair_Str(
              (Xen_Instance*)Xen_VM()->globals_instances->ptr,
              (Xen_Map_Pair_Str){mod->mod_name, mod_inst})) {
        return 0;
      }
    }
  }
  return 1;
}
