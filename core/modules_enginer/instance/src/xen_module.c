#include "callable.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "run_frame.h"
#include "vm.h"
#include "vm_def.h"
#include "xen_command.h"
#include "xen_map.h"
#include "xen_module.h"
#include "xen_module_implement.h"
#include "xen_module_instance.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_vector.h"

Xen_Instance *Xen_Module_New(Xen_Instance *mod_map, Xen_Instance *mod_context) {
  Xen_Module *module = (Xen_Module *)__instance_new(&Xen_Module_Implement, nil, 0);
  if (!module) { return nil; }
  module->mod_map = Xen_ADD_REF(mod_map);
  module->mod_context = Xen_ADD_REF(mod_context);
  return (Xen_Instance *)module;
}

Xen_Instance *Xen_Module_From_Def(struct Xen_Module_Def mod_def) {
  Xen_Instance *ctx_args =
      Xen_Vector_From_Array_With_Size(4, (Xen_Instance *[]){nil, nil, nil, nil});
  if_nil_eval(ctx_args) { return nil; }
  Xen_Instance *mod_ctx = __instance_new(&Xen_Run_Frame, ctx_args, 0);
  if_nil_eval(mod_ctx) {
    Xen_DEL_REF(ctx_args);
    return nil;
  }
  Xen_DEL_REF(ctx_args);
  RunContext_ptr ctx = (RunContext_ptr)mod_ctx;
  if (mod_def.mod_init) {
    ctx->ctx_code = callable_new_native(mod_def.mod_init);
    if (!ctx->ctx_code) {
      Xen_DEL_REF(mod_ctx);
      return nil;
    }
  }
  Xen_Instance *mod_map = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if_nil_eval(mod_map) {
    callable_free(ctx->ctx_code);
    Xen_DEL_REF(mod_ctx);
    return nil;
  }
  for (int i = 0; mod_def.mod_commands[i].cmd_name != NULL; i++) {
    Xen_Instance *cmd =
        Xen_Command_From_Native(mod_def.mod_commands[i].cmd_func, nil, mod_ctx);
    if_nil_eval(cmd) {
      Xen_DEL_REF(mod_map);
      callable_free(ctx->ctx_code);
      Xen_DEL_REF(mod_ctx);
      return nil;
    }
    if (!Xen_Map_Push_Pair_Str(
            mod_map, (Xen_Map_Pair_Str){mod_def.mod_commands[i].cmd_name, cmd})) {
      Xen_DEL_REF(cmd);
      Xen_DEL_REF(mod_map);
      callable_free(ctx->ctx_code);
      Xen_DEL_REF(mod_ctx);
      return nil;
    }
    Xen_DEL_REF(cmd);
  }
  Xen_Instance *module = Xen_Module_New(mod_map, mod_ctx);
  if_nil_eval(module) {
    Xen_DEL_REF(mod_map);
    callable_free(ctx->ctx_code);
    Xen_DEL_REF(mod_ctx);
    return nil;
  }
  if (mod_def.mod_init) {
    vm_run_ctx(ctx);
    if (ctx->ctx_reg.reg[1] != 1) {
      Xen_DEL_REF(module);
      Xen_DEL_REF(mod_map);
      callable_free(ctx->ctx_code);
      Xen_DEL_REF(mod_ctx);
      return nil;
    }
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
