#include "xen_method.h"
#include "attrs.h"
#include "instance.h"
#include "run_ctx_stack.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_except_instance.h"
#include "xen_function_instance.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_method_instance.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

Xen_Instance* Xen_Method_New(Xen_Instance* function, Xen_Instance* self) {
  if (!function || !self) {
    return NULL;
  }
  if (Xen_IMPL(function) != xen_globals->implements->function) {
    return NULL;
  }
  Xen_Method* method =
      (Xen_Method*)__instance_new(xen_globals->implements->method, nil, nil, 0);
  if (!method) {
    return NULL;
  }
  Xen_IGC_WRITE_FIELD(method, method->function, function);
  Xen_IGC_WRITE_FIELD(method, method->self, self);
  return (Xen_Instance*)method;
}

Xen_Instance* Xen_Method_Call(Xen_Instance* method_inst, Xen_Instance* args,
                              Xen_Instance* kwargs) {
  if (Xen_IMPL(method_inst) != xen_globals->implements->method) {
    return NULL;
  }
  Xen_size_t roots = 0;
  Xen_Method* method = (Xen_Method*)method_inst;
  Xen_Function_ptr fun = (Xen_Function_ptr)method->function;
  Xen_Instance* ret = NULL;
  if (fun->fun_type == 1) {
    Xen_Instance* fun_ctx = Xen_Ctx_New(nil, fun->closure, method->self, args,
                                        kwargs, NULL, fun->fun_code);
    if (!run_context_stack_push(&(*xen_globals->vm)->vm_ctx_stack, fun_ctx)) {
      return NULL;
    }
    Xen_Instance* args_list = Xen_Map_Keys(fun->args_names);
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
      if (!Xen_Map_Push_Pair(((RunContext_ptr)fun_ctx)->ctx_instances,
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
        if (!Xen_Map_Has(fun->args_names, keyword)) {
          run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
          return NULL;
        }
        if (Xen_Map_Has(((RunContext_ptr)fun_ctx)->ctx_instances, keyword)) {
          run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
          return NULL;
        }
        Xen_Instance* value = Xen_Map_Get(kwargs, keyword);
        if (!Xen_Map_Push_Pair(((RunContext_ptr)fun_ctx)->ctx_instances,
                               (Xen_Map_Pair){keyword, value})) {
          run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
          return NULL;
        }
      }
      if (!Xen_VM_Except_Active() ||
          strcmp(((Xen_Except*)(*xen_globals->vm)->except.except)->type,
                 "RangeEnd") != 0) {
        run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
        return NULL;
      }
      (*xen_globals->vm)->except.active = 0;
    }
    Xen_Instance* defaults_it = Xen_Attr_Iter(fun->args_default_values);
    if (!defaults_it) {
      run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
      return NULL;
    }
    Xen_IGC_XPUSH(defaults_it, roots);
    Xen_Instance* default_name = NULL;
    while ((default_name = Xen_Attr_Next(defaults_it)) != NULL) {
      if (!Xen_Map_Has(((RunContext_ptr)fun_ctx)->ctx_instances,
                       default_name)) {
        Xen_Instance* default_value =
            Xen_Map_Get(fun->args_default_values, default_name);
        if (!Xen_Map_Push_Pair(((RunContext_ptr)fun_ctx)->ctx_instances,
                               (Xen_Map_Pair){default_name, default_value})) {
          run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
          Xen_IGC_XPOP(roots);
          return NULL;
        }
      }
    }
    if (!Xen_VM_Except_Active() ||
        strcmp(((Xen_Except*)(*xen_globals->vm)->except.except)->type,
               "RangeEnd") != 0) {
      run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
      Xen_IGC_XPOP(roots);
      return NULL;
    }
    (*xen_globals->vm)->except.active = 0;
    for (Xen_ssize_t i = 0; i < fun->args_requireds; i++) {
      Xen_Instance* name = Xen_Vector_Get_Index(args_list, i);
      if (!Xen_Map_Has(((RunContext_ptr)fun_ctx)->ctx_instances, name)) {
        run_context_stack_pop_top(&(*xen_globals->vm)->vm_ctx_stack);
        return NULL;
      }
    }
    ret = vm_run(((RunContext_ptr)fun_ctx)->ctx_id);
    if (!ret) {
      Xen_IGC_XPOP(roots);
      return NULL;
    }
  } else if (fun->fun_type == 2) {
    ret = fun->fun_native(method->self, args, kwargs);
    if (!ret) {
      return NULL;
    }
  }
  return ret;
}

Xen_Instance* Xen_Method_Attr_Call(Xen_Instance* inst, Xen_Instance* attr,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  if (!inst || !attr || !args) {
    return NULL;
  }
  Xen_Instance* method = Xen_Attr_Get(inst, attr);
  if (!method) {
    return NULL;
  }
  Xen_IGC_Push(method);
  Xen_Instance* ret = Xen_Method_Call(method, args, kwargs);
  if (!ret) {
    Xen_IGC_Pop();
    return NULL;
  }
  Xen_IGC_Pop();
  return ret;
}

Xen_Instance* Xen_Method_Attr_Str_Call(Xen_Instance* inst, const char* attr,
                                       Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  if (!inst || !attr || !args) {
    return NULL;
  }
  Xen_Instance* attr_inst = Xen_String_From_CString(attr);
  if (!attr_inst) {
    return NULL;
  }
  Xen_IGC_Push(attr_inst);
  Xen_Instance* ret = Xen_Method_Attr_Call(inst, attr_inst, args, kwargs);
  if (!ret) {
    Xen_IGC_Pop();
    return NULL;
  }
  Xen_IGC_Pop();
  return ret;
}
