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
#include "xen_function.h"
#include "xen_map.h"
#include "xen_nil.h"

Xen_Instance* Xen_VM_Current_Ctx() {
  return run_context_stack_peek_top(&vm->vm_ctx_stack);
}

Xen_Instance* Xen_VM_Root_Ctx() {
  return (Xen_Instance*)vm->root_context;
}

bool Xen_VM_Store_Global(const char* name, Xen_Instance* val) {
  return Xen_Map_Push_Pair_Str(vm->root_context->ctx_instances,
                               (Xen_Map_Pair_Str){name, val});
}

bool Xen_VM_Store_Native_Function(Xen_Instance* inst_map, const char* name,
                                  Xen_Native_Func fun, Xen_Instance* closure) {
  Xen_INSTANCE* fun_inst = Xen_Function_From_Native(fun, closure);
  if (!fun_inst) {
    return false;
  }
  if (!Xen_Map_Push_Pair_Str(inst_map, (Xen_Map_Pair_Str){name, fun_inst})) {
    return false;
  }
  return true;
}

Xen_Instance* Xen_VM_Call_Native_Function(Xen_Native_Func func,
                                          Xen_INSTANCE* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  if (!run_context_stack_push(&vm->vm_ctx_stack, Xen_VM_Current_Ctx(),
                              (Xen_Instance*)vm->root_context, self, args,
                              kwargs)) {
    return NULL;
  }
  Xen_Instance* ret =
      func(run_ctx_id(Xen_VM_Current_Ctx()), self, args, kwargs);
  if (!ret) {
    run_context_stack_pop_top(&vm->vm_ctx_stack);
    return NULL;
  }
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  return ret;
}

Xen_INSTANCE* Xen_VM_Load_Instance(const char* name, ctx_id_t id) {
  if (!name || !VM_CHECK_ID(id)) {
    return NULL;
  }
  RunContext_ptr current = (RunContext_ptr)Xen_VM_Current_Ctx();
  while (current && Xen_Nil_NEval((Xen_Instance*)current)) {
    Xen_Instance* inst = Xen_Map_Get_Str(current->ctx_instances, name);
    if (inst != NULL) {
      return inst;
    }
    current = (RunContext_ptr)current->ctx_closure;
  }
  return NULL;
}

void Xen_VM_Ctx_Clear(RunContext_ptr ctx) {
  ctx->ctx_code = NULL;
  ctx->ctx_ip = 0;
  ctx->ctx_running = 0;
}

int Xen_VM_New_Ctx_Callable(CALLABLE_ptr callable, Xen_Instance* closure,
                            struct __Instance* self, Xen_Instance* args,
                            Xen_Instance* kwargs) {
  if (!callable) {
    return 0;
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack,
                              run_context_stack_peek_top(&vm->vm_ctx_stack),
                              closure, self, args, kwargs)) {
    return 0;
  }
  RunContext_ptr ctx =
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  ctx->ctx_code = callable;
  return 1;
}

Xen_Instance* Xen_VM_Call_Callable(CALLABLE_ptr callable, Xen_Instance* closure,
                                   struct __Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  if (!callable) {
    return NULL;
  }
  if (!Xen_VM_New_Ctx_Callable(callable, closure, self, args, kwargs)) {
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
