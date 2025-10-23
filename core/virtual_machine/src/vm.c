#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "callable.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "run_ctx_stack.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_command_implement.h"
#include "xen_command_instance.h"
#include "xen_map.h"
#include "xen_nil.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

Xen_Instance* vm_current_ctx() {
  return run_context_stack_peek_top(&vm->vm_ctx_stack);
}

Xen_Instance* vm_root_ctx() {
  return (Xen_Instance*)vm->root_context;
}

bool vm_define_native_command(Xen_Instance* inst_map, const char* name,
                              Xen_Native_Func fun) {
  Xen_INSTANCE* cmd_inst = __instance_new(&Xen_Command_Implement, nil, 0);
  if_nil_eval(cmd_inst) {
    return false;
  }
  struct Xen_Command_Instance* command_inst =
      (struct Xen_Command_Instance*)cmd_inst;
  command_inst->cmd_callable = callable_new_native(fun);
  if (!command_inst->cmd_callable) {
    Xen_DEL_REF(cmd_inst);
    return false;
  }
  if (!Xen_Map_Push_Pair_Str(inst_map, (Xen_Map_Pair_Str){name, cmd_inst})) {
    Xen_DEL_REF(cmd_inst);
    return false;
  }
  return true;
}

Xen_Instance* vm_call_native_function(Xen_Native_Func func, Xen_INSTANCE* self,
                                      Xen_Instance* args) {
  if (!run_context_stack_push(&vm->vm_ctx_stack, vm_current_ctx(),
                              (Xen_Instance*)vm->root_context, self, args)) {
    return NULL;
  }
  Xen_Instance* ret = func(run_ctx_id(vm_current_ctx()), self, args);
  if (!ret) {
    run_context_stack_pop_top(&vm->vm_ctx_stack);
    return NULL;
  }
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  return ret;
}

Xen_INSTANCE* vm_get_instance(const char* name, ctx_id_t id) {
  if (!name || !VM_CHECK_ID(id)) {
    return nil;
  }
  RunContext_ptr current = (RunContext_ptr)vm_current_ctx();
  while (current && Xen_Nil_NEval((Xen_Instance*)current)) {
    Xen_Instance* inst = Xen_Map_Get_Str(current->ctx_instances, name);
    if_nil_neval(inst) {
      return inst;
    }
    current = (RunContext_ptr)current->ctx_closure;
  }
  return nil;
}

void vm_ctx_clear(RunContext_ptr ctx) {
  ctx->ctx_code = NULL;
  ctx->ctx_ip = 0;
  ctx->ctx_running = 0;
}

int vm_new_ctx_callable(CALLABLE_ptr callable, Xen_Instance* closure,
                        struct __Instance* self, Xen_Instance* args) {
  if (!callable) {
    return 0;
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack,
                              run_context_stack_peek_top(&vm->vm_ctx_stack),
                              closure, self, args)) {
    return 0;
  }
  RunContext_ptr ctx =
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  ctx->ctx_code = callable;
  return 1;
}

Xen_Instance* vm_run_callable(CALLABLE_ptr callable, Xen_Instance* closure,
                              struct __Instance* self, Xen_Instance* args) {
  if (!callable) {
    return NULL;
  }
  if (!vm_new_ctx_callable(callable, closure, self, args)) {
    return NULL;
  }
  Xen_Instance* ret =
      vm_run_ctx((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack));
  if (!ret) {
    run_context_stack_pop_top(&vm->vm_ctx_stack);
    return NULL;
  }
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  return ret;
}
