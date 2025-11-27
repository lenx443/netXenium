#include "xen_method_implement.h"
#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "xen_function_instance.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_method_instance.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

static void method_trace(Xen_GCHeader* h) {
  Xen_Method* method = (Xen_Method*)h;
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)method->function);
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)method->self);
}

static Xen_Instance* method_alloc(struct __Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Method* method = (Xen_Method*)Xen_Instance_Alloc(&Xen_Method_Implement);
  if (!method) {
    return NULL;
  }
  method->function = nil;
  method->self = nil;
  return (Xen_Instance*)method;
}

static Xen_Instance* method_destroy(struct __Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return nil;
}

static Xen_Instance* method_string(struct __Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_String_From_CString("<Method>");
}

static Xen_Instance* method_callable(struct __Instance* self,
                                     Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Method* method = (Xen_Method*)self;
  Xen_Function_ptr function = (Xen_Function_ptr)method->function;
  if (function->fun_type == 1) {
    Xen_Instance* new_ctx = Xen_Ctx_New(
        run_context_stack_peek_top(&vm->vm_ctx_stack), function->closure,
        method->self, args, kwargs, NULL, function->fun_code);
    if (!run_context_stack_push(&vm->vm_ctx_stack, new_ctx)) {
      return NULL;
    }
    Xen_Instance* args_list = Xen_Map_Keys(function->args_names);
    if (Xen_SIZE(args) > Xen_SIZE(args_list)) {
      run_context_stack_pop_top(&vm->vm_ctx_stack);
      return NULL;
    }
    for (Xen_size_t i = 0; i < Xen_SIZE(args); i++) {
      Xen_Instance* name = Xen_Vector_Get_Index(args_list, i);
      Xen_Instance* arg = Xen_Vector_Get_Index(args, i);
      if (!name) {
        run_context_stack_pop_top(&vm->vm_ctx_stack);
        return NULL;
      }
      if (!Xen_Map_Push_Pair(((RunContext_ptr)new_ctx)->ctx_instances,
                             (Xen_Map_Pair){name, arg})) {
        run_context_stack_pop_top(&vm->vm_ctx_stack);
        return NULL;
      }
    }
    if (Xen_IMPL(kwargs) == &Xen_Map_Implement) {
      Xen_Instance* kwargs_it = Xen_Attr_Iter(kwargs);
      if (!kwargs_it) {
        run_context_stack_pop_top(&vm->vm_ctx_stack);
        return NULL;
      }
      Xen_Instance* keyword = NULL;
      while ((keyword = Xen_Attr_Next(kwargs_it)) != NULL) {
        if (!Xen_Map_Has(function->args_names, keyword)) {
          run_context_stack_pop_top(&vm->vm_ctx_stack);
          return NULL;
        }
        if (Xen_Map_Has(((RunContext_ptr)new_ctx)->ctx_instances, keyword)) {
          run_context_stack_pop_top(&vm->vm_ctx_stack);
          return NULL;
        }
        Xen_Instance* value = Xen_Map_Get(kwargs, keyword);
        if (!Xen_Map_Push_Pair(((RunContext_ptr)new_ctx)->ctx_instances,
                               (Xen_Map_Pair){keyword, value})) {
          run_context_stack_pop_top(&vm->vm_ctx_stack);
          return NULL;
        }
      }
    }
    Xen_Instance* defaults_it = Xen_Attr_Iter(function->args_default_values);
    if (!defaults_it) {
      run_context_stack_pop_top(&vm->vm_ctx_stack);
      return NULL;
    }
    Xen_Instance* default_name = NULL;
    while ((default_name = Xen_Attr_Next(defaults_it)) != NULL) {
      if (!Xen_Map_Has(((RunContext_ptr)new_ctx)->ctx_instances,
                       default_name)) {
        Xen_Instance* default_value =
            Xen_Map_Get(function->args_default_values, default_name);
        if (!Xen_Map_Push_Pair(((RunContext_ptr)new_ctx)->ctx_instances,
                               (Xen_Map_Pair){default_name, default_value})) {
          run_context_stack_pop_top(&vm->vm_ctx_stack);
          return NULL;
        }
      }
    }
    for (Xen_ssize_t i = 0; i < function->args_requireds; i++) {
      Xen_Instance* name = Xen_Vector_Get_Index(args_list, i);
      if (!Xen_Map_Has(((RunContext_ptr)new_ctx)->ctx_instances, name)) {
        run_context_stack_pop_top(&vm->vm_ctx_stack);
        return NULL;
      }
    }
  } else if (function->fun_type == 2) {
    Xen_Instance* ret = function->fun_native(method->self, args, kwargs);
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

Xen_Implement Xen_Method_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Method",
    .__inst_size = sizeof(struct Xen_Method_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = method_trace,
    .__props = &Xen_Nil_Def,
    .__alloc = method_alloc,
    .__create = NULL,
    .__destroy = method_destroy,
    .__string = method_string,
    .__raw = method_string,
    .__callable = method_callable,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};
