#include "xen_module.h"
#include "callable.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_function.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"

Xen_Instance* Xen_Module_New(Xen_Instance* mod_map, Xen_Instance* mod_context) {
  Xen_Module* module =
      (Xen_Module*)__instance_new(&Xen_Module_Implement, nil, nil, 0);
  if (!module) {
    return NULL;
  }
  Xen_IGC_WRITE_FIELD(module, module->mod_map, mod_map);
  Xen_IGC_WRITE_FIELD(module, module->mod_context, mod_context);
  return (Xen_Instance*)module;
}

Xen_Instance* Xen_Module_From_Def(struct Xen_Module_Def mod_def) {
  Xen_Instance* mod_ctx = Xen_Ctx_New(nil, nil, nil, nil, nil, NULL,
                                      callable_new_native(mod_def.mod_init));
  if (!mod_ctx) {
    return NULL;
  }
  RunContext_ptr ctx = (RunContext_ptr)mod_ctx;
  if (mod_def.mod_init) {
    ctx->ctx_code = callable_new_native(mod_def.mod_init);
    if (!ctx->ctx_code) {
      return NULL;
    }
  }
  Xen_Instance* mod_map = Xen_Map_New();
  if (!mod_map) {
    callable_free(ctx->ctx_code);
    return NULL;
  }
  for (int i = 0; mod_def.mod_functions[i].fun_name != NULL; i++) {
    Xen_Instance* fun =
        Xen_Function_From_Native(mod_def.mod_functions[i].fun_func, mod_ctx);
    if (!fun) {
      callable_free(ctx->ctx_code);
      return NULL;
    }
    if (!Xen_Map_Push_Pair_Str(
            mod_map,
            (Xen_Map_Pair_Str){mod_def.mod_functions[i].fun_name, fun})) {
      callable_free(ctx->ctx_code);
      return NULL;
    }
  }
  Xen_Instance* module = Xen_Module_New(mod_map, mod_ctx);
  if (!module) {
    callable_free(ctx->ctx_code);
    return NULL;
  }
  if (mod_def.mod_init) {
    Xen_Instance* ret = vm_run_ctx(ctx);
    if (ret == NULL) {
      callable_free(ctx->ctx_code);
      return NULL;
    }
    callable_free(ctx->ctx_code);
    ctx->ctx_code = NULL;
  }
  if (!Xen_Map_Push_Pair_Str(vm->modules_contexts,
                             (Xen_Map_Pair_Str){mod_def.mod_name, mod_ctx})) {
    return NULL;
  }
  return module;
}
