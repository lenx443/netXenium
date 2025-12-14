#include <dlfcn.h>
#include <stdio.h>

#include "compiler.h"
#include "instance.h"
#include "list.h"
#include "macros.h"
#include "string_utf8.h"
#include "vm.h"
#include "vm_run.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_function.h"
#include "xen_map.h"
#include "xen_module.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

typedef struct Xen_Module_Def* (*native_module_function)(void);

Xen_Instance* Xen_Module_New(void) {
  Xen_Module* module =
      (Xen_Module*)__instance_new(&Xen_Module_Implement, nil, nil, 0);
  module->mod_initialized = 0;
  module->mod_initializing = 0;
  module->mod_name = NULL;
  module->mod_path = NULL;
  return (Xen_Instance*)module;
}

Xen_Instance* Xen_Module_From_Def(struct Xen_Module_Def mod_def,
                                  Xen_c_string_t mod_path) {
  Xen_Module* module = (Xen_Module*)Xen_Map_Get_Str((*xen_globals->vm)->modules,
                                                    mod_def.mod_name);
  if (Xen_Map_Has_Str((*xen_globals->vm)->modules, mod_def.mod_name)) {
    if (module->mod_initialized) {
      return (Xen_Instance*)module;
    }
    if (module->mod_initializing) {
      return (Xen_Instance*)module;
    }
  }
  module = (Xen_Module*)Xen_Module_New();
  module->mod_initializing = 1;
  module->mod_name = Xen_CString_Dup(mod_def.mod_name);
  module->mod_path = Xen_CString_Dup(mod_path);
  Xen_Map_Push_Pair_Str(
      (*xen_globals->vm)->modules,
      (Xen_Map_Pair_Str){mod_def.mod_name, (Xen_Instance*)module});
  if (mod_def.mod_functions) {
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
  }
  if (mod_def.mod_init) {
    if (!Xen_Vector_Push((*xen_globals->vm)->modules_stack,
                         (Xen_Instance*)module)) {
      return NULL;
    }
    Xen_Instance* ret =
        Xen_VM_Call_Native_Function(mod_def.mod_init, nil, nil, nil);
    if (ret == NULL) {
      Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
      return NULL;
    }
    Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
  }
  module->mod_initialized = 0;
  return (Xen_Instance*)module;
}

Xen_Instance* Xen_Module_Load(Xen_c_string_t mod_name, Xen_c_string_t mod_uname,
                              Xen_c_string_t mod_path,
                              Xen_Instance* mod_globals, Xen_uint8_t mod_type) {
  if (mod_type == XEN_MODULE_GUEST) {
    Xen_Module* module =
        (Xen_Module*)Xen_Map_Get_Str((*xen_globals->vm)->modules, mod_name);
    if (Xen_Map_Has_Str((*xen_globals->vm)->modules, mod_name)) {
      if (module->mod_initialized) {
        return (Xen_Instance*)module;
      }
      if (module->mod_initializing) {
        return (Xen_Instance*)module;
      }
    }
    module = (Xen_Module*)Xen_Module_New();
    module->mod_initializing = 1;
    module->mod_name = Xen_CString_Dup(mod_uname);
    module->mod_path = Xen_CString_Dup(mod_path);
    Xen_Map_Push_Pair_Str((*xen_globals->vm)->modules,
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
    if (!Xen_Vector_Push((*xen_globals->vm)->modules_stack,
                         (Xen_Instance*)module)) {
      return NULL;
    }
    CALLABLE_ptr code = compiler(mod_name, file_content, Xen_COMPILE_PROGRAM);
    if (Xen_VM_Except_Active()) {
      Xen_VM_Except_Backtrace_Show();
      Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
      return NULL;
    }
    if (!code) {
      Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
      return NULL;
    }
#ifndef NDEBUG
    printf("== Running ==\n");
#endif
    Xen_Instance* ctx_inst =
        Xen_Ctx_New(nil, Xen_VM_Current_Ctx(), (Xen_Instance*)module, nil, nil,
                    mod_globals, code);
    if (!ctx_inst) {
      Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
      return NULL;
    }
    if (!run_context_stack_push(&(*xen_globals->vm)->vm_ctx_stack, ctx_inst)) {
      Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
      return NULL;
    }
    Xen_Instance* retval = vm_run_top();
    if (Xen_VM_Except_Active()) {
      Xen_VM_Except_Backtrace_Show();
      Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
      return NULL;
    }
    if (!retval) {
      Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
      return NULL;
    }
    Xen_Vector_Pop((*xen_globals->vm)->modules_stack);
    Xen_Dealloc(file_content);
    return (Xen_Instance*)module;
  } else if (mod_type == XEN_MODULE_NATIVE) {
    void* handle = dlopen(mod_name, RTLD_LAZY);
    if (!handle) {
      dlerror();
      return NULL;
    }
    Xen_c_string_t format_string = "Xen_Module_%s_Start";
    Xen_ssize_t fsize = snprintf(NULL, 0, format_string, mod_uname);
    if (fsize == -1) {
      return NULL;
    }
    Xen_string_t func_name = Xen_Alloc(fsize + 1);
    snprintf(func_name, fsize + 1, format_string, mod_uname);
    native_module_function mod_start =
        (native_module_function)dlsym(handle, func_name);
    if (dlerror() != NULL) {
      dlclose(handle);
      return NULL;
    }
    struct Xen_Module_Def* mod_def = mod_start();
    Xen_Instance* module = Xen_Module_From_Def(*mod_def, mod_path);
    Xen_Dealloc(mod_def);
    dlclose(handle);
    return module;
  }
  return NULL;
}
