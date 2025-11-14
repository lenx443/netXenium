#include <string.h>

#include "attrs.h"
#include "basic_templates.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_boolean_implement.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_tuple.h"
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
      Xen_VM_Call_Native_Function(Xen_IMPL(inst)->__get_attr, inst, args, nil);
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

int Xen_Attr_Set(Xen_Instance* inst, Xen_Instance* attr, Xen_Instance* val) {
  if (!inst || !attr || !val) {
    return 0;
  }
  if (Xen_IMPL(attr) != &Xen_String_Implement) {
    return 0;
  }
  if (!Xen_IMPL(inst)->__set_attr) {
    return 0;
  }
  Xen_Instance* args = Xen_Vector_From_Array(2, (Xen_Instance*[]){attr, val});
  if (!args) {
    return 0;
  }
  Xen_Instance* result =
      Xen_VM_Call_Native_Function(Xen_IMPL(inst)->__set_attr, inst, args, nil);
  if (!result) {
    Xen_DEL_REF(args);
    return 0;
  }
  Xen_DEL_REF(result);
  Xen_DEL_REF(args);
  return 1;
}

int Xen_Attr_Set_Str(Xen_Instance* inst, const char* attr, Xen_Instance* val) {
  if (!inst || !attr || !val) {
    return 0;
  }
  Xen_Instance* attr_inst = Xen_String_From_CString(attr);
  if (!attr_inst) {
    return 0;
  }
  if (!Xen_Attr_Set(inst, attr_inst, val)) {
    Xen_DEL_REF(attr_inst);
    return 0;
  }
  Xen_DEL_REF(attr_inst);
  return 1;
}

Xen_Instance* Xen_Attr_String(Xen_Instance* inst) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* string = Xen_Method_Attr_Str_Call(inst, "__string", nil, nil);
  if (!string) {
    if (Xen_IMPL(inst)->__string == NULL) {
      string = Xen_VM_Call_Native_Function(Xen_Basic_String, inst, nil, nil);
      if (!string) {
        return NULL;
      }
    } else {
      string =
          Xen_VM_Call_Native_Function(Xen_IMPL(inst)->__string, inst, nil, nil);
      if (!string) {
        return NULL;
      }
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
  const char* string_str = strdup(Xen_String_As_CString(string));
  if (!string_str) {
    Xen_DEL_REF(string);
    return NULL;
  }
  Xen_DEL_REF(string);
  return string_str;
}

Xen_Instance* Xen_Attr_String_Stack(Xen_Instance* inst, Xen_Instance* stack) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(1, &stack);
  if (!args) {
    return NULL;
  }
  Xen_Instance* string = Xen_Method_Attr_Str_Call(inst, "__string", args, nil);
  if (!string) {
    if (Xen_IMPL(inst)->__string == NULL) {
      string = Xen_VM_Call_Native_Function(Xen_Basic_String, inst, args, nil);
      if (!string) {
        Xen_DEL_REF(args);
        return NULL;
      }
    } else {
      string = Xen_VM_Call_Native_Function(Xen_IMPL(inst)->__string, inst, args,
                                           nil);
      if (!string) {
        Xen_DEL_REF(args);
        return NULL;
      }
    }
  }
  Xen_DEL_REF(args);
  if (Xen_IMPL(string) != &Xen_String_Implement) {
    Xen_DEL_REF(string);
    return NULL;
  }
  return string;
}

const char* Xen_Attr_String_Stack_Str(Xen_Instance* inst, Xen_Instance* stack) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* string = Xen_Attr_String_Stack(inst, stack);
  if (!string) {
    return NULL;
  }
  const char* string_str = strdup(Xen_String_As_CString(string));
  if (!string_str) {
    Xen_DEL_REF(string);
    return NULL;
  }
  Xen_DEL_REF(string);
  return string_str;
}

Xen_Instance* Xen_Attr_Raw(Xen_Instance* inst) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* raw = Xen_Method_Attr_Str_Call(inst, "__raw", nil, nil);
  if (!raw) {
    if (Xen_IMPL(inst)->__raw == NULL) {
      raw = Xen_VM_Call_Native_Function(Xen_Basic_String, inst, nil, nil);
      if (!raw) {
        return NULL;
      }
    } else {
      raw = Xen_VM_Call_Native_Function(Xen_IMPL(inst)->__raw, inst, nil, nil);
      if (!raw) {
        return NULL;
      }
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
  const char* raw_str = strdup(Xen_String_As_CString(raw));
  if (!raw_str) {
    Xen_DEL_REF(raw);
    return NULL;
  }
  Xen_DEL_REF(raw);
  return raw_str;
}

Xen_Instance* Xen_Attr_Raw_Stack(Xen_Instance* inst, Xen_Instance* stack) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(1, &stack);
  if (!args) {
    return NULL;
  }
  Xen_Instance* raw = Xen_Method_Attr_Str_Call(inst, "__raw", args, nil);
  if (!raw) {
    if (Xen_IMPL(inst)->__string == NULL) {
      raw = Xen_VM_Call_Native_Function(Xen_Basic_String, inst, args, nil);
      if (!raw) {
        Xen_DEL_REF(args);
        return NULL;
      }
    } else {
      raw = Xen_VM_Call_Native_Function(Xen_IMPL(inst)->__raw, inst, args, nil);
      if (!raw) {
        Xen_DEL_REF(args);
        return NULL;
      }
    }
  }
  Xen_DEL_REF(args);
  if (Xen_IMPL(raw) != &Xen_String_Implement) {
    Xen_DEL_REF(raw);
    return NULL;
  }
  return raw;
}

const char* Xen_Attr_Raw_Stack_Str(Xen_Instance* inst, Xen_Instance* stack) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* raw = Xen_Attr_Raw_Stack(inst, stack);
  if (!raw) {
    return NULL;
  }
  const char* raw_str = strdup(Xen_String_As_CString(raw));
  if (!raw_str) {
    Xen_DEL_REF(raw);
    return NULL;
  }
  Xen_DEL_REF(raw);
  return raw_str;
}

Xen_Instance* Xen_Attr_Boolean(Xen_Instance* inst) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* boolean = Xen_Method_Attr_Str_Call(inst, "__boolean", nil, nil);
  if (!boolean) {
    return NULL;
  }
  if (Xen_IMPL(boolean) != &Xen_Boolean_Implement) {
    Xen_DEL_REF(boolean);
    return NULL;
  }
  return boolean;
}

Xen_Instance* Xen_Attr_Index_Get(Xen_Instance* inst, Xen_Instance* index) {
  if (!inst || !index) {
    return NULL;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(1, &index);
  if (!args) {
    return NULL;
  }
  Xen_Instance* ret = Xen_Method_Attr_Str_Call(inst, "__get_index", args, nil);
  if (!ret) {
    Xen_DEL_REF(args);
    return NULL;
  }
  Xen_DEL_REF(args);
  return ret;
}

Xen_Instance* Xen_Attr_Index_Size_Get(Xen_Instance* inst, Xen_size_t index) {
  if (!inst) {
    return NULL;
  }
  Xen_Instance* index_inst = Xen_Number_From_ULongLong(index);
  if (!index_inst) {
    return NULL;
  }
  Xen_Instance* ret = Xen_Attr_Index_Get(inst, index_inst);
  if (!ret) {
    Xen_DEL_REF(index_inst);
    return NULL;
  }
  Xen_DEL_REF(index_inst);
  return ret;
}

int Xen_Attr_Index_Set(Xen_Instance* inst, Xen_Instance* index,
                       Xen_Instance* val) {
  if (!inst || !index) {
    return 0;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(2, (Xen_Instance*[]){index, val});
  if (!args) {
    return 0;
  }
  Xen_Instance* ret = Xen_Method_Attr_Str_Call(inst, "__set_index", args, nil);
  if (!ret) {
    Xen_DEL_REF(args);
    return 0;
  }
  Xen_DEL_REF(args);
  Xen_DEL_REF(ret);
  return 1;
}

int Xen_Attr_Index_Size_Set(Xen_Instance* inst, Xen_size_t index,
                            Xen_Instance* val) {
  if (!inst) {
    return 0;
  }
  Xen_Instance* index_inst = Xen_Number_From_ULongLong(index);
  if (!index_inst) {
    return 0;
  }
  if (!Xen_Attr_Index_Set(inst, index_inst, val)) {
    Xen_DEL_REF(index_inst);
    return 0;
  }
  Xen_DEL_REF(index_inst);
  return 1;
}

Xen_Instance* Xen_Attr_Iter(Xen_Instance* iterable) {
  if (!iterable) {
    return NULL;
  }
  Xen_Instance* iter = Xen_Method_Attr_Str_Call(iterable, "__iter", nil, nil);
  if (!iter) {
    return NULL;
  }
  return iter;
}

Xen_Instance* Xen_Attr_Next(Xen_Instance* iter) {
  if (!iter) {
    return NULL;
  }
  Xen_Instance* rsult = Xen_Method_Attr_Str_Call(iter, "__next", nil, nil);
  if (!rsult) {
    return NULL;
  }
  return rsult;
}
