#include "xen_function_implement.h"
#include "attrs.h"
#include "basic.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "run_ctx_stack.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_stack.h"
#include "xen_except_instance.h"
#include "xen_function_instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static void function_trace(Xen_GCHeader* h) {
  Xen_Function_ptr inst = (Xen_Function_ptr)h;
  if (inst->fun_type == 1) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)inst->fun_code->ptr);
  }
  if (inst->closure->ptr) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)inst->closure->ptr);
  }
  if (inst->args_names->ptr) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)inst->args_names->ptr);
  }
  if (inst->args_default_values->ptr) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)inst->args_default_values->ptr);
  }
}

static Xen_Instance* function_alloc(struct __Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ptr inst =
      (Xen_Function_ptr)Xen_Instance_Alloc(xen_globals->implements->function);
  if (!inst) {
    return NULL;
  }
  inst->fun_type = 0;
  inst->fun_code = Xen_GCHandle_New();
  inst->fun_native = NULL;
  inst->closure = Xen_GCHandle_New_From((Xen_GCHeader*)nil);
  inst->args_names = Xen_GCHandle_New_From((Xen_GCHeader*)nil);
  inst->args_default_values = Xen_GCHandle_New_From((Xen_GCHeader*)nil);
  inst->args_requireds = -1;
  return (Xen_Instance*)inst;
}

static Xen_Instance* function_destroy(struct __Instance* self,
                                      Xen_Instance* args,
                                      Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ptr inst = (Xen_Function_ptr)self;
  Xen_GCHandle_Free(inst->fun_code);
  Xen_GCHandle_Free(inst->closure);
  Xen_GCHandle_Free(inst->args_names);
  Xen_GCHandle_Free(inst->args_default_values);
  return nil;
}

static Xen_Instance* function_callable(struct __Instance* self,
                                       Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ptr inst = (Xen_Function_ptr)self;
  if (inst->fun_type == 1) {
    Xen_Instance* new_ctx = Xen_Ctx_New(
        run_context_stack_peek_top(&(*xen_globals->vm)->vm_ctx_stack),
        (Xen_Instance*)inst->closure->ptr, nil, args, kwargs, NULL,
        (CALLABLE_ptr)inst->fun_code->ptr);
    if (!run_context_stack_push(&(*xen_globals->vm)->vm_ctx_stack, new_ctx)) {
      return NULL;
    }
    Xen_Instance* args_list =
        Xen_Map_Keys((Xen_Instance*)inst->args_names->ptr);
    if (Xen_SIZE(args) > Xen_SIZE(args_list)) {
      run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
      return NULL;
    }
    for (Xen_size_t i = 0; i < Xen_SIZE(args); i++) {
      Xen_Instance* name = Xen_Vector_Get_Index(args_list, i);
      Xen_Instance* arg = Xen_Vector_Get_Index(args, i);
      if (!name) {
        run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
        return NULL;
      }
      if (!Xen_Map_Push_Pair(
              (Xen_Instance*)((RunContext_ptr)new_ctx)->ctx_instances->ptr,
              (Xen_Map_Pair){name, arg})) {
        run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
        return NULL;
      }
    }
    if (Xen_IMPL(kwargs) == xen_globals->implements->map) {
      Xen_Instance* kwargs_it = Xen_Attr_Iter(kwargs);
      if (!kwargs_it) {
        run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
        return NULL;
      }
      Xen_Instance* keyword = NULL;
      while ((keyword = Xen_Attr_Next(kwargs_it)) != NULL) {
        if (!Xen_Map_Has((Xen_Instance*)inst->args_names->ptr, keyword)) {
          run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
          return NULL;
        }
        if (Xen_Map_Has(
                (Xen_Instance*)((RunContext_ptr)new_ctx)->ctx_instances->ptr,
                keyword)) {
          run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
          return NULL;
        }
        Xen_Instance* value = Xen_Map_Get(kwargs, keyword);
        if (!Xen_Map_Push_Pair(
                (Xen_Instance*)((RunContext_ptr)new_ctx)->ctx_instances->ptr,
                (Xen_Map_Pair){keyword, value})) {
          run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
          return NULL;
        }
      }
      if (!Xen_VM_Except_Active() ||
          strcmp(((Xen_Except*)(*xen_globals->vm)->except.except->ptr)->type,
                 "RangeEnd") != 0) {
        run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
        return NULL;
      }
      (*xen_globals->vm)->except.active = 0;
    }
    Xen_Instance* defaults_it =
        Xen_Attr_Iter((Xen_Instance*)inst->args_default_values->ptr);
    if (!defaults_it) {
      run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
      return NULL;
    }
    Xen_Instance* default_name = NULL;
    while ((default_name = Xen_Attr_Next(defaults_it)) != NULL) {
      if (!Xen_Map_Has(
              (Xen_Instance*)((RunContext_ptr)new_ctx)->ctx_instances->ptr,
              default_name)) {
        Xen_Instance* default_value = Xen_Map_Get(
            (Xen_Instance*)inst->args_default_values->ptr, default_name);
        if (!Xen_Map_Push_Pair(
                (Xen_Instance*)((RunContext_ptr)new_ctx)->ctx_instances->ptr,
                (Xen_Map_Pair){default_name, default_value})) {
          run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
          return NULL;
        }
      }
    }
    if (!Xen_VM_Except_Active() ||
        strcmp(((Xen_Except*)(*xen_globals->vm)->except.except->ptr)->type,
               "RangeEnd") != 0) {
      run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
      return NULL;
    }
    (*xen_globals->vm)->except.active = 0;
    for (Xen_ssize_t i = 0; i < inst->args_requireds; i++) {
      Xen_Instance* name = Xen_Vector_Get_Index(args_list, i);
      if (!Xen_Map_Has(
              (Xen_Instance*)((RunContext_ptr)new_ctx)->ctx_instances->ptr,
              name)) {
        run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
        return NULL;
      }
    }
  } else if (inst->fun_type == 2) {
    Xen_Instance* ret = inst->fun_native(nil, args, kwargs);
    if (!ret) {
      return NULL;
    }
    vm_stack_push((struct vm_Stack*)((RunContext_ptr)run_context_stack_peek_top(
                                         &(*xen_globals->vm)->vm_ctx_stack))
                      ->ctx_stack->ptr,
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

struct __Implement __Function_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Function",
    .__inst_size = sizeof(Xen_Function),
    .__inst_default_flags = 0x00,
    .__inst_trace = function_trace,
    .__props = NULL,
    .__alloc = function_alloc,
    .__create = NULL,
    .__destroy = function_destroy,
    .__string = function_string,
    .__raw = function_string,
    .__callable = function_callable,
    .__hash = NULL,
};

struct __Implement* Xen_Function_GetImplement(void) {
  return &__Function_Implement;
}
