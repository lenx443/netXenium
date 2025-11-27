#include "xen_function.h"
#include "attrs.h"
#include "callable.h"
#include "gc_header.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_function_implement.h"
#include "xen_function_instance.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_nil.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

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

Xen_INSTANCE*
Xen_Function_From_Callable(CALLABLE_ptr code_fun, Xen_Instance* closure,
                           Xen_Instance* args_names_list,
                           Xen_Instance* args_default_values_list) {
  if (Xen_SIZE(args_names_list) < Xen_SIZE(args_default_values_list)) {
    return NULL;
  }
  Xen_size_t roots = 0;
  Xen_Function* fun =
      (Xen_Function*)__instance_new(&Xen_Function_Implement, nil, nil, 0);
  if (!fun) {
    return NULL;
  }
  Xen_IGC_XPUSH((Xen_Instance*)fun, roots);
  fun->fun_type = 1;
  Xen_Instance* args_names = Xen_Map_New();
  if (!args_names) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_IGC_XPUSH(args_names, roots);
  if (Xen_Nil_NEval(args_names_list)) {
    Xen_Instance* args_names_it = Xen_Attr_Iter(args_names_list);
    if (!args_names_it) {
      Xen_IGC_XPOP(roots);
      return NULL;
    }
    Xen_IGC_XPUSH(args_names_it, roots);
    Xen_Instance* arg_name = NULL;
    while ((arg_name = Xen_Attr_Next(args_names_it)) != NULL) {
      if (!Xen_Map_Push_Pair(args_names, (Xen_Map_Pair){arg_name, nil})) {
        Xen_IGC_XPOP(roots);
        return NULL;
      }
    }
  }
  Xen_Instance* args_default_values = Xen_Map_New();
  if (!args_default_values) {
    Xen_IGC_XPOP(roots);
    return NULL;
  }
  Xen_IGC_XPUSH(args_default_values, roots);
  if (Xen_SIZE(args_default_values_list) != 0) {
    Xen_size_t start_index =
        Xen_SIZE(args_names_list) - Xen_SIZE(args_default_values_list);
    for (Xen_size_t i = 0; i < Xen_SIZE(args_default_values_list); i++) {
      Xen_Instance* default_name =
          Xen_Tuple_Get_Index(args_names_list, start_index + i);
      Xen_Instance* default_value =
          Xen_Tuple_Get_Index(args_default_values_list, i);
      if (!Xen_Map_Push_Pair(args_default_values,
                             (Xen_Map_Pair){default_name, default_value})) {
        Xen_IGC_XPOP(roots);
        return NULL;
      }
    }
  }
  Xen_GC_Write_Field((Xen_GCHeader*)fun, (Xen_GCHeader**)&fun->fun_code,
                     (Xen_GCHeader*)code_fun);
  Xen_IGC_WRITE_FIELD(fun, fun->closure, closure);
  Xen_IGC_WRITE_FIELD(fun, fun->args_names, args_names);
  Xen_IGC_WRITE_FIELD(fun, fun->args_default_values, args_default_values);
  fun->args_requireds =
      Xen_SIZE(args_names) - Xen_SIZE(args_default_values_list);
  Xen_IGC_XPOP(roots);
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
    Xen_Instance* args_list = Xen_Map_Keys(fun->args_names);
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
      if (!Xen_Map_Push_Pair(((RunContext_ptr)fun_ctx)->ctx_instances,
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
        if (!Xen_Map_Has(fun->args_names, keyword)) {
          run_context_stack_pop_top(&vm->vm_ctx_stack);
          return NULL;
        }
        if (Xen_Map_Has(((RunContext_ptr)fun_ctx)->ctx_instances, keyword)) {
          run_context_stack_pop_top(&vm->vm_ctx_stack);
          return NULL;
        }
        Xen_Instance* value = Xen_Map_Get(kwargs, keyword);
        if (!Xen_Map_Push_Pair(((RunContext_ptr)fun_ctx)->ctx_instances,
                               (Xen_Map_Pair){keyword, value})) {
          run_context_stack_pop_top(&vm->vm_ctx_stack);
          return NULL;
        }
      }
    }
    Xen_Instance* defaults_it = Xen_Attr_Iter(fun->args_default_values);
    if (!defaults_it) {
      run_context_stack_pop_top(&vm->vm_ctx_stack);
      return NULL;
    }
    Xen_Instance* default_name = NULL;
    while ((default_name = Xen_Attr_Next(defaults_it)) != NULL) {
      if (!Xen_Map_Has(((RunContext_ptr)fun_ctx)->ctx_instances,
                       default_name)) {
        Xen_Instance* default_value =
            Xen_Map_Get(fun->args_default_values, default_name);
        if (!Xen_Map_Push_Pair(((RunContext_ptr)fun_ctx)->ctx_instances,
                               (Xen_Map_Pair){default_name, default_value})) {
          run_context_stack_pop_top(&vm->vm_ctx_stack);
          return NULL;
        }
      }
    }
    for (Xen_ssize_t i = 0; i < fun->args_requireds; i++) {
      Xen_Instance* name = Xen_Vector_Get_Index(args_list, i);
      if (!Xen_Map_Has(((RunContext_ptr)fun_ctx)->ctx_instances, name)) {
        run_context_stack_pop_top(&vm->vm_ctx_stack);
        return NULL;
      }
    }
    ret = vm_run(((RunContext_ptr)fun_ctx)->ctx_id);
    if (!ret) {
      return NULL;
    }
  } else if (fun->fun_type == 2) {
    ret = fun->fun_native(nil, args, kwargs);
    if (!ret) {
      return NULL;
    }
  }
  return ret;
}
