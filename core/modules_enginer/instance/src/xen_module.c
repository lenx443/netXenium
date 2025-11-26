#include "xen_module.h"
#include "instance.h"
#include "vm.h"
#include "xen_function.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"

Xen_Instance* Xen_Module_New(Xen_Instance* mod_map) {
  Xen_Module* module =
      (Xen_Module*)__instance_new(&Xen_Module_Implement, nil, nil, 0);
  if (!module) {
    return NULL;
  }
  Xen_IGC_WRITE_FIELD(module, module->mod_map, mod_map);
  return (Xen_Instance*)module;
}

Xen_Instance* Xen_Module_From_Def(struct Xen_Module_Def mod_def) {
  Xen_Instance* mod_map = Xen_Map_New();
  if (!mod_map) {
    return NULL;
  }
  for (int i = 0; mod_def.mod_functions[i].fun_name != NULL; i++) {
    Xen_Instance* fun =
        Xen_Function_From_Native(mod_def.mod_functions[i].fun_func, nil);
    if (!fun) {
      return NULL;
    }
    if (!Xen_Map_Push_Pair_Str(
            mod_map,
            (Xen_Map_Pair_Str){mod_def.mod_functions[i].fun_name, fun})) {
      return NULL;
    }
  }
  Xen_Instance* module = Xen_Module_New(mod_map);
  if (!module) {
    return NULL;
  }
  if (mod_def.mod_init) {
    Xen_Instance* ret =
        Xen_VM_Call_Native_Function(mod_def.mod_init, nil, nil, nil);
    if (ret == NULL) {
      return NULL;
    }
  }
  return module;
}
