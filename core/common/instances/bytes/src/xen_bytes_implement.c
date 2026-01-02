#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_bytes.h"
#include "xen_bytes_implement.h"
#include "xen_bytes_instance.h"
#include "xen_cstrings.h"
#include "xen_function.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

static Xen_Instance* bytes_create(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  for (Xen_size_t i = 0; i < Xen_SIZE(args); i++) {
    Xen_Instance* val = Xen_Tuple_Get_Index(args, i);
    if (Xen_IMPL(val) != xen_globals->implements->bytes) {
      return NULL;
    }
    Xen_Bytes_Append_Array(self, Xen_SIZE(val), ((Xen_Bytes*)val)->bytes);
  }
  return nil;
}

static Xen_Instance* bytes_string(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Bytes* bytes = (Xen_Bytes*)self;
  Xen_string_t buffer = Xen_CString_Dup("<Bytes('");
  Xen_size_t buflen = 10;
  for (Xen_size_t i = 0; i < Xen_SIZE(bytes); i++) {
    char byte[4];
    static const char HEX[] = "0123456789ABCDEF";
    byte[0] = '\\';
    byte[1] = 'x';
    byte[2] = HEX[(bytes->bytes[i] >> 4) & 0xF];
    byte[3] = HEX[bytes->bytes[i] & 0xF];
    buflen += 4;
    buffer = Xen_Realloc(buffer, buflen);
    strncat(buffer, byte, 4);
  }
  buflen += 3;
  buffer = Xen_Realloc(buffer, buflen);
  strcat(buffer, "')>");
  buffer[buflen - 1] = '\0';
  Xen_Instance* string = Xen_String_From_CString(buffer);
  Xen_Dealloc(buffer);
  return string;
}

static Xen_Instance* bytes_opr_mul(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* num_inst = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(num_inst) != xen_globals->implements->number)
    return NULL;
  Xen_size_t num = Xen_Number_As(Xen_size_t, num_inst);
  Xen_Instance* result = Xen_Bytes_New();
  for (Xen_size_t i = 0; i < num; i++) {
    Xen_Bytes_Append_Array(result, Xen_SIZE(self), ((Xen_Bytes*)self)->bytes);
  }
  return result;
}

static Xen_Instance* bytes_append(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  Xen_Function_ArgSpec args_def[] = {
      {"bytes", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_BYTES,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* bytes = Xen_Function_ArgBinding_Search(binding, "bytes")->value;
  Xen_Function_ArgBinding_Free(binding);
  Xen_Bytes_Append_Array(self, Xen_SIZE(bytes), ((Xen_Bytes*)bytes)->bytes);
  return nil;
}

static Xen_Instance* bytes_prop_string(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Xen_Bytes* bytes = (Xen_Bytes*)self;
  Xen_string_t buffer = Xen_Alloc(Xen_SIZE(bytes) + 1);
  for (Xen_size_t i = 0; i < Xen_SIZE(bytes); i++) {
    buffer[i] = (char)bytes->bytes[i];
  }
  buffer[Xen_SIZE(bytes)] = '\0';
  Xen_Instance* string = Xen_String_From_CString(buffer);
  Xen_Dealloc(buffer);
  return string;
}

static Xen_Instance* bytes_slice(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  Xen_Function_ArgSpec args_def[] = {
      {"first", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_REQUIRED, NULL},
      {"second", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_NUMBER,
       XEN_FUNCTION_ARG_OPTIONAL, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_bool_t is_pair = 0;
  Xen_ssize_t first = Xen_Number_As_LongLong(
      Xen_Function_ArgBinding_Search(binding, "first")->value);
  Xen_Function_ArgBound* second_arg =
      Xen_Function_ArgBinding_Search(binding, "second");
  Xen_size_t second = 0;
  if (second_arg->provided) {
    is_pair = 1;
    second = Xen_Number_As_ULongLong(second_arg->value);
  }
  Xen_Function_ArgBinding_Free(binding);
  Xen_Bytes* bytes = (Xen_Bytes*)self;
  if (is_pair) {
    if (first < 0 || (Xen_size_t)first > second ||
        (Xen_size_t)first > Xen_SIZE(bytes) || second > Xen_SIZE(bytes)) {
      return NULL;
    }
    Xen_Instance* result =
        Xen_Bytes_From_Array(second - first, bytes->bytes + first);
    return result;
  } else {
    if (first >= 0) {
      if ((Xen_size_t)first > Xen_SIZE(bytes)) {
        return NULL;
      }
      Xen_Instance* result =
          Xen_Bytes_From_Array(Xen_SIZE(bytes) - first, bytes->bytes + first);
      return result;
    } else {
      if (first < INT_MIN || first > INT_MAX) {
        return NULL;
      }
      int abs_first = abs((int)first);
      if ((Xen_size_t)abs_first > Xen_SIZE(bytes)) {
        return NULL;
      }
      Xen_Instance* result = Xen_Bytes_From_Array(abs_first, bytes->bytes);
      return result;
    }
  }
  return NULL;
}

static Xen_Instance* bytes_signed(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  Xen_Function_ArgSpec args_def[] = {
      {"big_endian", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_BOOLEAN, XEN_FUNCTION_ARG_OPTIONAL, Xen_True},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* big_ending =
      Xen_Function_ArgBinding_Search(binding, "big_endian")->value;
  Xen_Function_ArgBinding_Free(binding);
  Xen_Bytes* bytes = (Xen_Bytes*)self;
  if (big_ending == Xen_True) {
    Xen_Instance* num =
        Xen_Number_From_Bytes(bytes->bytes, bytes->__size, 1, 1);
    return num;
  } else {
    Xen_Instance* num =
        Xen_Number_From_Bytes(bytes->bytes, bytes->__size, 1, 0);
    return num;
  }
}

static Xen_Instance* bytes_unsigned(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  Xen_Function_ArgSpec args_def[] = {
      {"big_endian", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_BOOLEAN, XEN_FUNCTION_ARG_OPTIONAL, Xen_True},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* big_ending =
      Xen_Function_ArgBinding_Search(binding, "big_endian")->value;
  Xen_Function_ArgBinding_Free(binding);
  Xen_Bytes* bytes = (Xen_Bytes*)self;
  if (big_ending == Xen_True) {
    Xen_Instance* num =
        Xen_Number_From_Bytes(bytes->bytes, bytes->__size, 0, 1);
    return num;
  } else {
    Xen_Instance* num =
        Xen_Number_From_Bytes(bytes->bytes, bytes->__size, 0, 0);
    return num;
  }
}

static Xen_Implement __Bytes_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Bytes",
    .__inst_size = sizeof(struct Xen_Bytes_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = NULL,
    .__props = NULL,
    .__base = NULL,
    .__alloc = NULL,
    .__create = bytes_create,
    .__destroy = NULL,
    .__string = bytes_string,
    .__raw = bytes_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
    .__set_attr = NULL,
};

struct __Implement* Xen_Bytes_GetImplement(void) {
  return &__Bytes_Implement;
}

int Xen_Bytes_Init(void) {
  if (!Xen_VM_Store_Global("bytes",
                           (Xen_Instance*)xen_globals->implements->bytes)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  Xen_VM_Store_Native_Function(props, "__mul", bytes_opr_mul, nil);
  Xen_VM_Store_Native_Function(props, "append", bytes_append, nil);
  Xen_VM_Store_Native_Function(props, "string", bytes_prop_string, nil);
  Xen_VM_Store_Native_Function(props, "slice", bytes_slice, nil);
  Xen_VM_Store_Native_Function(props, "signed", bytes_signed, nil);
  Xen_VM_Store_Native_Function(props, "unsigned", bytes_unsigned, nil);
  Xen_IGC_Fork_Push(impls_maps, props);
  __Bytes_Implement.__props = props;
  return 1;
}

void Xen_Bytes_Finish(void) {}
