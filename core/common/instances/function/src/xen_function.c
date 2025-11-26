#include "xen_function.h"
#include "callable.h"
#include "gc_header.h"
#include "instance.h"
#include "program_code.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_function_implement.h"
#include "xen_function_instance.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_nil.h"

Xen_INSTANCE* Xen_Function_From_Native(Xen_Native_Func fn_fun,
                                       Xen_Instance* closure) {
  Xen_Function* fun =
      (Xen_Function*)__instance_new(&Xen_Function_Implement, nil, nil, 0);
  if (!fun) {
    return NULL;
  }
  fun->fun_type = 2;
  fun->fun_native = fn_fun;
  if_nil_neval(closure) {
    Xen_GC_Write_Field((Xen_GCHeader*)fun, (Xen_GCHeader**)&fun->closure,
                       (Xen_GCHeader*)closure);
  }
  return (Xen_INSTANCE*)fun;
}

Xen_INSTANCE* Xen_Function_From_Program(ProgramCode_t pc_fun,
                                        Xen_Instance* closure) {
  Xen_Function* fun =
      (Xen_Function*)__instance_new(&Xen_Function_Implement, nil, nil, 0);
  if (!fun) {
    return NULL;
  }
  fun->fun_type = 1;
  fun->fun_code = callable_new(pc_fun);
  if (!fun->fun_code) {
    return NULL;
  }
  if_nil_neval(closure) {
    Xen_IGC_WRITE_FIELD(fun, fun->closure, closure);
  }
  return (Xen_INSTANCE*)fun;
}

Xen_Instance* Xen_Function_Call(Xen_Instance* fun_inst, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  if (Xen_IMPL(fun_inst) != &Xen_Function_Implement) {
    return NULL;
  }
  Xen_Function_ptr fun = (Xen_Function_ptr)fun_inst;
  Xen_Instance* ret = NULL;
  if (fun->fun_type == 1) {
    Xen_Instance* fun_ctx =
        Xen_Ctx_New(nil, fun->closure, nil, args, kwargs, NULL, fun->fun_code);
    if (!run_context_stack_push(&vm->vm_ctx_stack, fun_ctx)) {
      return NULL;
    }
    ret = vm_run(((RunContext_ptr)fun_ctx)->ctx_id);
    if (!ret) {
      return NULL;
    }
  } else if (fun->fun_type == 2) {
    ret = fun->fun_native(0, nil, args, kwargs);
    if (!ret) {
      return NULL;
    }
  }
  return ret;
}
