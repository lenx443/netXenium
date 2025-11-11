#include "xen_module.h"
#include "callable.h"
#include "instance.h"
#include "run_frame.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_function.h"
#include "xen_map.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_vector.h"

Xen_Instance* Xen_Module_New(Xen_Instance* mod_map, Xen_Instance* mod_context) {
  Xen_Module* module =
      (Xen_Module*)__instance_new(&Xen_Module_Implement, nil, nil, 0);
  if (!module) {
    return NULL;
  }
  module->mod_map = Xen_ADD_REF(mod_map);
  module->mod_context = Xen_ADD_REF(mod_context);
  return (Xen_Instance*)module;
}

Xen_Instance* Xen_Module_From_Def(struct Xen_Module_Def mod_def) {
  Xen_Instance* ctx_args =
      Xen_Vector_From_Array(5, (Xen_Instance*[]){nil, nil, nil, nil, nil});
  if (!ctx_args) {
    return NULL;
  }
  Xen_Instance* mod_ctx = __instance_new(&Xen_Run_Frame, ctx_args, nil, 0);
  if (!mod_ctx) {
    Xen_DEL_REF(ctx_args);
    return NULL;
  }
  Xen_DEL_REF(ctx_args);
  RunContext_ptr ctx = (RunContext_ptr)mod_ctx;
  if (mod_def.mod_init) {
    ctx->ctx_code = callable_new_native(mod_def.mod_init);
    if (!ctx->ctx_code) {
      Xen_DEL_REF(mod_ctx);
      return NULL;
    }
  }
  Xen_Instance* mod_map = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if (!mod_map) {
    callable_free(ctx->ctx_code);
    Xen_DEL_REF(mod_ctx);
    return NULL;
  }
  for (int i = 0; mod_def.mod_functions[i].fun_name != NULL; i++) {
    Xen_Instance* fun =
        Xen_Function_From_Native(mod_def.mod_functions[i].fun_func, mod_ctx);
    if (!fun) {
      Xen_DEL_REF(mod_map);
      callable_free(ctx->ctx_code);
      Xen_DEL_REF(mod_ctx);
      return NULL;
    }
    if (!Xen_Map_Push_Pair_Str(
            mod_map,
            (Xen_Map_Pair_Str){mod_def.mod_functions[i].fun_name, fun})) {
      Xen_DEL_REF(fun);
      Xen_DEL_REF(mod_map);
      callable_free(ctx->ctx_code);
      Xen_DEL_REF(mod_ctx);
      return NULL;
    }
    Xen_DEL_REF(fun);
  }
  Xen_Instance* module = Xen_Module_New(mod_map, mod_ctx);
  if (!module) {
    Xen_DEL_REF(mod_map);
    callable_free(ctx->ctx_code);
    Xen_DEL_REF(mod_ctx);
    return NULL;
  }
  if (mod_def.mod_init) {
    Xen_Instance* ret = vm_run_ctx(ctx);
    if (ret == NULL) {
      Xen_DEL_REF(module);
      Xen_DEL_REF(mod_map);
      callable_free(ctx->ctx_code);
      Xen_DEL_REF(mod_ctx);
      return NULL;
    }
    Xen_DEL_REF(ret);
    callable_free(ctx->ctx_code);
    ctx->ctx_code = NULL;
  }
  if (!Xen_Map_Push_Pair_Str(vm->modules_contexts,
                             (Xen_Map_Pair_Str){mod_def.mod_name, mod_ctx})) {
    Xen_DEL_REF(mod_map);
    Xen_DEL_REF(mod_ctx);
  }
  Xen_DEL_REF(mod_map);
  Xen_DEL_REF(mod_ctx);
  return module;
}
