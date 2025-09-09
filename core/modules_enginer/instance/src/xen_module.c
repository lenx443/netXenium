#include "instance.h"
#include "run_ctx_stack.h"
#include "vm.h"
#include "vm_def.h"
#include "xen_command.h"
#include "xen_map.h"
#include "xen_module.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"

Xen_Instance *Xen_Module_New(Xen_Instance *mod_map, Xen_Instance *mod_context) {
  Xen_Module *module = (Xen_Module *)__instance_new(&Xen_Module_Implement, nil, 0);
  if (!module) { return nil; }
  module->mod_map = Xen_ADD_REF(mod_map);
  module->mod_context = Xen_ADD_REF(mod_context);
  return (Xen_Instance *)module;
}

Xen_Instance *Xen_Module_From_Def(struct Xen_Module_Def mod_def) {
  if (!run_context_stack_push(&vm->vm_ctx_stack, nil, nil, nil, nil)) { return nil; }
  Xen_Instance *mod_context = vm_current_ctx();
  Xen_Instance *mod_map = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if_nil_eval(mod_map) { return nil; }
  for (int i = 0; mod_def.mod_commands[i].cmd_name != NULL; i++) {
    Xen_Instance *cmd =
        Xen_Command_From_Native(mod_def.mod_commands[i].cmd_func, nil, mod_context);
    if_nil_eval(cmd) {
      Xen_DEL_REF(mod_map);
      run_context_stack_pop_top(&vm->vm_ctx_stack);
      return nil;
    }
    if (!Xen_Map_Push_Pair_Str(
            mod_map, (Xen_Map_Pair_Str){mod_def.mod_commands[i].cmd_name, cmd})) {
      Xen_DEL_REF(cmd);
      Xen_DEL_REF(mod_map);
      run_context_stack_pop_top(&vm->vm_ctx_stack);
      return nil;
    }
    Xen_DEL_REF(cmd);
  }
  Xen_Instance *module = Xen_Module_New(mod_map, mod_context);
  if_nil_eval(module) {
    Xen_DEL_REF(mod_map);
    run_context_stack_pop_top(&vm->vm_ctx_stack);
  }
  Xen_DEL_REF(mod_map);
  return module;
}
