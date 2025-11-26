#include "xen_function_implement.h"
#include "basic.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "vm_stack.h"
#include "xen_function_instance.h"
#include "xen_gc.h"
#include "xen_nil.h"
#include "xen_string.h"

static void function_trace(Xen_GCHeader* h) {
  Xen_Function_ptr inst = (Xen_Function_ptr)h;
  if (inst->fun_type == 1) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)inst->fun_code);
  }
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)inst->closure);
}

static Xen_Instance* function_alloc(struct __Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ptr inst =
      (Xen_Function_ptr)Xen_Instance_Alloc(&Xen_Function_Implement);
  if (!inst) {
    return NULL;
  }
  inst->fun_type = 0;
  inst->fun_code = NULL;
  inst->fun_native = NULL;
  inst->closure = nil;
  return (Xen_Instance*)inst;
}

static Xen_Instance* function_destroy(struct __Instance* self,
                                      Xen_Instance* args,
                                      Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  return nil;
}

static Xen_Instance* function_callable(struct __Instance* self,
                                       Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ptr inst = (Xen_Function_ptr)self;
  if (inst->fun_type == 1) {
    Xen_Instance* new_ctx =
        Xen_Ctx_New(run_context_stack_peek_top(&vm->vm_ctx_stack),
                    inst->closure, nil, args, kwargs, NULL, inst->fun_code);
    if (!run_context_stack_push(&vm->vm_ctx_stack, new_ctx)) {
      return NULL;
    }
  } else if (inst->fun_type == 2) {
    Xen_Instance* ret = inst->fun_native(nil, args, kwargs);
    if (!ret) {
      return NULL;
    }
    vm_stack_push(
        ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack))
            ->ctx_stack,
        ret);
  }
  return nil;
}

static Xen_Instance* function_string(Xen_Instance* self, Xen_Instance* args,
                                     Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Instance* string = Xen_String_From_CString("<Function>");
  if (!string) {
    return NULL;
  }
  return string;
}

struct __Implement Xen_Function_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Function",
    .__inst_size = sizeof(Xen_Function),
    .__inst_default_flags = 0x00,
    .__inst_trace = function_trace,
    .__props = &Xen_Nil_Def,
    .__alloc = function_alloc,
    .__create = NULL,
    .__destroy = function_destroy,
    .__string = function_string,
    .__raw = function_string,
    .__callable = function_callable,
    .__hash = NULL,
};
