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
  if (Xen_IMPL(attr) != &Xen_String_Implement) {
    return NULL;
  }
  if (!Xen_IMPL(inst)->__get_attr) {
    return NULL;
  }
  Xen_Instance* args = Xen_Vector_From_Array(1, &attr);
  if (!args) {
    return NULL;
  }
  Xen_Instance* result =
      vm_call_native_function(Xen_IMPL(inst)->__get_attr, inst, args);
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
  if (!attr_inst) {
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

Xen_Instance* Xen_Attr_String(Xen_Instance* inst) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* string = Xen_Attr_Get_Str(inst, "__string");
  if (!string) {
    if (Xen_IMPL(inst)->__string == NULL) {
      return NULL;
    }
    string = vm_call_native_function(Xen_IMPL(inst)->__string, inst, nil);
    if (!string) {
      return NULL;
    }
  }
  if (Xen_IMPL(string) != &Xen_String_Implement) {
    Xen_DEL_REF(string);
    return NULL;
  }
  return string;
}

const char* Xen_Attr_String_Str(Xen_Instance* inst) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* string = Xen_Attr_String(inst);
  if (!string) {
    return NULL;
  }
  return Xen_String_As_CString(string);
}

Xen_Instance* Xen_Attr_Raw(Xen_Instance* inst) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* raw = Xen_Attr_Get_Str(inst, "__raw");
  if (!raw) {
    if (Xen_IMPL(inst)->__string == NULL) {
      return NULL;
    }
    raw = vm_call_native_function(Xen_IMPL(inst)->__raw, inst, nil);
    if (!raw) {
      return NULL;
    }
  }
  if (Xen_IMPL(raw) != &Xen_String_Implement) {
    Xen_DEL_REF(raw);
    return NULL;
  }
  return raw;
}

const char* Xen_Attr_Raw_Str(Xen_Instance* inst) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* raw = Xen_Attr_Raw(inst);
  if (!raw) {
    return NULL;
  }
  return Xen_String_As_CString(raw);
}
