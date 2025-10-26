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
  if (Xen_TYPE(function) != &Xen_Function_Implement) {
    return NULL;
  }
  Xen_Method* method =
      (Xen_Method*)__instance_new(&Xen_Method_Implement, nil, 0);
  if_nil_eval(method) {
    return NULL;
  }
  method->function = Xen_ADD_REF(function);
  method->self = Xen_ADD_REF(self);
  return (Xen_Instance*)method;
}
