#include "xen_method.h"
#include "instance.h"
#include "xen_function_implement.h"
#include "xen_method_implement.h"
#include "xen_method_instance.h"
#include "xen_nil.h"

Xen_Instance* Xen_Method_New(Xen_Instance* function, Xen_Instance* self) {
  if (!function || !self) {
    return NULL;
  }
  if (Xen_IMPL(function) != &Xen_Function_Implement) {
    return NULL;
  }
  Xen_Method* method =
      (Xen_Method*)__instance_new(&Xen_Method_Implement, nil, 0);
  if (!method) {
    return NULL;
  }
  method->function = Xen_ADD_REF(function);
  method->self = Xen_ADD_REF(self);
  return (Xen_Instance*)method;
}

Xen_Instance* Xen_Method_Call(Xen_Instance* method, Xen_Instance* args) {
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    return NULL;
  }
  Xen_Instance* ret = Xen_Method_Implement.__callable(0, method, args);
  if (!ret) {
    return NULL;
  }
  return ret;
}
