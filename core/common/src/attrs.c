#include "attrs.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_vector.h"

Xen_Instance* Xen_Attr_Get(Xen_Instance* inst, Xen_Instance* attr) {
  if (!inst || !attr) {
    return NULL;
  }
  if (Xen_TYPE(attr) != &Xen_String_Implement) {
    return NULL;
  }
  if (!Xen_TYPE(inst)->__get_attr) {
    return NULL;
  }
  Xen_Instance* args = Xen_Vector_From_Array(1, &attr);
  if_nil_eval(args) {
    return NULL;
  }
  Xen_Instance* result =
      vm_call_native_function(Xen_TYPE(inst)->__get_attr, inst, args);
  if (!result) {
    Xen_DEL_REF(args);
    return NULL;
  }
  Xen_DEL_REF(args);
  return result;
}

Xen_Instance* Xen_Attr_Get_Str(Xen_Instance* inst, const char* attr) {
  if (!inst || !attr) {
    return NULL;
  }
  Xen_Instance* attr_inst = Xen_String_From_CString(attr);
  if_nil_eval(attr_inst) {
    return NULL;
  }
  Xen_Instance* result = Xen_Attr_Get(inst, attr_inst);
  if (!result) {
    Xen_DEL_REF(attr_inst);
    return NULL;
  }
  Xen_DEL_REF(attr_inst);
  return result;
}
