#include "xen_function.h"
#include "callable.h"
#include "instance.h"
#include "program_code.h"
#include "xen_function_implement.h"
#include "xen_function_instance.h"
#include "xen_nil.h"

Xen_INSTANCE* Xen_Function_From_Native(Xen_Native_Func fn_fun,
                                       Xen_Instance* closure) {
  Xen_Function* fun =
      (Xen_Function*)__instance_new(&Xen_Function_Implement, nil, 0);
  if (!fun) {
    return NULL;
  }
  fun->fun_callable = callable_new_native(fn_fun);
  if (!fun->fun_callable) {
    Xen_DEL_REF(fun);
    return NULL;
  }
  if_nil_neval(closure) fun->closure = Xen_ADD_REF(closure);
  return (Xen_INSTANCE*)fun;
}

Xen_INSTANCE* Xen_Function_From_Program(ProgramCode_t pc_fun,
                                        Xen_Instance* closure) {
  Xen_Function* fun =
      (Xen_Function*)__instance_new(&Xen_Function_Implement, nil, 0);
  if (!fun) {
    return NULL;
  }
  fun->fun_callable = callable_new_code(pc_fun);
  if (!fun->fun_callable) {
    Xen_DEL_REF(fun);
    return NULL;
  }
  if_nil_neval(closure) fun->closure = Xen_ADD_REF(closure);
  return (Xen_INSTANCE*)fun;
}

Xen_Instance* Xen_Function_Call(Xen_Instance* fun, Xen_Instance* args) {
  if (Xen_IMPL(fun) != &Xen_Function_Implement) {
    return NULL;
  }
  Xen_Instance* ret = Xen_Function_Implement.__callable(0, fun, args);
  if (!ret) {
    return NULL;
  }
  return ret;
}
