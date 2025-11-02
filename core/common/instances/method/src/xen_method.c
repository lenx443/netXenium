#include "xen_method.h"
#include "attrs.h"
#include "instance.h"
#include "xen_function_implement.h"
#include "xen_method_implement.h"
#include "xen_method_instance.h"
#include "xen_nil.h"
#include "xen_string.h"

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

Xen_Instance* Xen_Method_Attr_Call(Xen_Instance* inst, Xen_Instance* attr,
                                   Xen_Instance* args) {
  if (!inst || !attr || !args) {
    return NULL;
  }
  Xen_Instance* method = Xen_Attr_Get(inst, attr);
  if (!method) {
    return NULL;
  }
  Xen_Instance* ret = Xen_Method_Call(method, args);
  if (!ret) {
    Xen_DEL_REF(method);
    return NULL;
  }
  Xen_DEL_REF(method);
  return ret;
}

Xen_Instance* Xen_Method_Attr_Str_Call(Xen_Instance* inst, const char* attr,
                                       Xen_Instance* args) {
  if (!inst || !attr || !args) {
    return NULL;
  }
  Xen_Instance* attr_inst = Xen_String_From_CString(attr);
  if (!attr_inst) {
    return NULL;
  }
  Xen_Instance* ret = Xen_Method_Attr_Call(inst, attr_inst, args);
  if (!ret) {
    Xen_DEL_REF(attr_inst);
    return NULL;
  }
  Xen_DEL_REF(attr_inst);
  return ret;
}
