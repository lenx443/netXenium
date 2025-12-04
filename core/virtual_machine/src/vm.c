#include <assert.h>
#include <stdio.h>

#include "callable.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "run_ctx_stack.h"
#include "source_file.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_except_implement.h"
#include "xen_except_instance.h"
#include "xen_function.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_VM_Current_Ctx(void) {
  return run_context_stack_peek_top(&vm->vm_ctx_stack);
}

bool Xen_VM_Store_Global(const char* name, Xen_Instance* val) {
  return Xen_Map_Push_Pair_Str(vm->globals_instances,
                               (Xen_Map_Pair_Str){name, val});
}

bool Xen_VM_Store_Native_Function(Xen_Instance* inst_map, const char* name,
                                  Xen_Native_Func fun, Xen_Instance* closure) {
  Xen_INSTANCE* fun_inst = Xen_Function_From_Native(fun, closure);
  if (!fun_inst) {
    return false;
  }
  Xen_IGC_Push(fun_inst);
  if (!Xen_Map_Push_Pair_Str(inst_map, (Xen_Map_Pair_Str){name, fun_inst})) {
    Xen_IGC_Pop();
    return false;
  }
  Xen_IGC_Pop();
  return true;
}

Xen_Instance* Xen_VM_Call_Native_Function(Xen_Native_Func func,
                                          Xen_INSTANCE* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  Xen_Instance* ret = func(self, args, kwargs);
  if (!ret) {
    return NULL;
  }
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
  Xen_Instance* ctx_inst =
      Xen_Ctx_New(run_context_stack_peek_top(&vm->vm_ctx_stack)
                      ? run_context_stack_peek_top(&vm->vm_ctx_stack)
                      : nil,
                  closure, self, args, kwargs, NULL, callable);
  if (!ctx_inst) {
    return 0;
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack, ctx_inst)) {
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
  Xen_Instance* ret = vm_run_top();
  if (!ret) {
    return NULL;
  }
  return ret;
}

void Xen_VM_Except_Show(Xen_Source_Address* bt, Xen_size_t bt_count) {
  Xen_Except* except = (Xen_Except*)vm->except.except;
  puts("Unhandled exception occurred.");
  puts("BackTrace:");
  for (Xen_size_t i = 0; i < bt_count; i++) {
    printf("file: \"%s\"; line: %ld; column: %ld;\n",
           globals_sources->st_files[(Xen_size_t)bt[i].id]->sf_name, bt[i].line,
           bt->column);
  }
  if (except->message) {
    fputs(except->type, stdout);
    fputs(": ", stdout);
    puts(except->message);
  } else {
    fputs("Type: ", stdout);
    puts(except->type);
  }
  vm->except.active = 0;
}

int Xen_VM_Except_Throw(Xen_Instance* except_inst) {
  assert(except_inst != NULL);
  Xen_Instance* except =
      Xen_Method_Attr_Str_Call(except_inst, "__except", nil, nil);
  if (!except) {
    return 0;
  }
  if (Xen_IMPL(except) != &Xen_Except_Implement) {
    return 0;
  }
  vm->except.active = 1;
  vm->except.except = except;
  return 1;
}
