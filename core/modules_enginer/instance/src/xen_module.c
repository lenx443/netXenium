#include "xen_module.h"
#include "compiler.h"
#include "instance.h"
#include "list.h"
#include "macros.h"
#include "string_utf8.h"
#include "vm.h"
#include "vm_run.h"
#include "xen_alloc.h"
#include "xen_function.h"
#include "xen_map.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"

Xen_Instance* Xen_Module_New(void) {
  Xen_Module* module =
      (Xen_Module*)__instance_new(&Xen_Module_Implement, nil, nil, 0);
  module->mod_initialized = 0;
  module->mod_initializing = 0;
  return (Xen_Instance*)module;
}

Xen_Instance* Xen_Module_From_Def(struct Xen_Module_Def mod_def) {
  Xen_Module* module =
      (Xen_Module*)Xen_Map_Get_Str(vm->modules, mod_def.mod_name);
  if (Xen_Map_Has_Str(vm->modules, mod_def.mod_name)) {
    if (module->mod_initialized) {
      return (Xen_Instance*)module;
    }
    if (module->mod_initializing) {
      return (Xen_Instance*)module;
    }
  }
  module = (Xen_Module*)Xen_Module_New();
  module->mod_initializing = 1;
  Xen_Map_Push_Pair_Str(
      vm->modules, (Xen_Map_Pair_Str){mod_def.mod_name, (Xen_Instance*)module});
  for (int i = 0; mod_def.mod_functions[i].fun_name != NULL; i++) {
    Xen_Instance* fun =
        Xen_Function_From_Native(mod_def.mod_functions[i].fun_func, nil);
    if (!fun) {
      return NULL;
    }
    if (!Xen_Map_Push_Pair_Str(
            module->__map,
            (Xen_Map_Pair_Str){mod_def.mod_functions[i].fun_name, fun})) {
      return NULL;
    }
  }
  if (mod_def.mod_init) {
    Xen_Instance* ret =
        Xen_VM_Call_Native_Function(mod_def.mod_init, nil, nil, nil);
    if (ret == NULL) {
      return NULL;
    }
  }
  module->mod_initialized = 0;
  return (Xen_Instance*)module;
}

Xen_Instance* Xen_Module_Load(const char* mod_name) {
  Xen_Module* module = (Xen_Module*)Xen_Map_Get_Str(vm->modules, mod_name);
  if (Xen_Map_Has_Str(vm->modules, mod_name)) {
    if (module->mod_initialized) {
      return (Xen_Instance*)module;
    }
    if (module->mod_initializing) {
      return (Xen_Instance*)module;
    }
  }
  module = (Xen_Module*)Xen_Module_New();
  module->mod_initializing = 1;
  Xen_Map_Push_Pair_Str(vm->modules,
                        (Xen_Map_Pair_Str){mod_name, (Xen_Instance*)module});
  FILE* fp = fopen(mod_name, "r");
  if (!fp) {
    return NULL;
  }
  char line[CMDSIZ];
  LIST_ptr buffer = list_new();
  if (!buffer) {
    return NULL;
  }
  while (fgets(line, CMDSIZ, fp)) {
    if (!string_utf8_push_back(buffer, line)) {
      list_free(buffer);
      fclose(fp);
      return NULL;
    }
  }
  fclose(fp);
  char* file_content = string_utf8_get(buffer);
  list_free(buffer);
  if (!file_content) {
    return NULL;
  }
  CALLABLE_ptr code = compiler(mod_name, file_content, Xen_COMPILE_PROGRAM);
  if (Xen_VM_Except_Active()) {
    Xen_VM_Except_Backtrace_Show();
    return NULL;
  }
  if (!code) {
    return NULL;
  }
#ifndef NDEBUG
  printf("== Running ==\n");
#endif
  Xen_Instance* ctx_inst = Xen_Ctx_New(
      nil, Xen_VM_Current_Ctx(), (Xen_Instance*)module, nil, nil, NULL, code);
  if (!ctx_inst) {
    return NULL;
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack, ctx_inst)) {
    return NULL;
  }
  Xen_Instance* retval = vm_run_top();
  if (Xen_VM_Except_Active()) {
    Xen_VM_Except_Backtrace_Show();
    return NULL;
  }
  if (!retval) {
    return NULL;
  }
  Xen_Dealloc(file_content);
  return (Xen_Instance*)module;
}
