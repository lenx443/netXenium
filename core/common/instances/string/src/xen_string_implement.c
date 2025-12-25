#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "instance_life.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_bytes.h"
#include "xen_cstrings.h"
#include "xen_function.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static Xen_Instance* string_alloc(Xen_INSTANCE* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* stack = NULL;
    if (kwargs && Xen_Nil_NEval(kwargs)) {
      stack = Xen_Map_Get_Str(kwargs, "stack");
    }
    Xen_Instance* val = Xen_Tuple_Get_Index(args, 0);
    Xen_Instance* rsult = NULL;
    if (stack) {
      rsult = Xen_Attr_String_Stack(val, stack);
    } else {
      rsult = Xen_Attr_String(val);
    }
    if (!rsult) {
      return NULL;
    }
    return rsult;
  }
  Xen_String* string =
      (Xen_String*)Xen_Instance_Alloc(xen_globals->implements->string);
  if (!string) {
    return NULL;
  }
  string->characters = NULL;
  return (Xen_Instance*)string;
}

static Xen_Instance* string_destroy(Xen_INSTANCE* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_String* string = (Xen_String*)self;
  if (string->characters)
    Xen_Dealloc(string->characters);
  return nil;
}

static Xen_Instance* string_string(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return self;
}

static Xen_Instance* string_raw(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_String* string = (Xen_String*)self;
  char* raw_string = Xen_CString_As_Raw(string->characters);
  if (!raw_string) {
    return NULL;
  }
  char* buffer = Xen_Alloc(Xen_CString_Len(raw_string) + 3);
  if (!buffer) {
    Xen_Dealloc(raw_string);
    return NULL;
  }
  strcpy(buffer, "'");
  strcat(buffer, raw_string);
  strcat(buffer, "'");
  Xen_Dealloc(raw_string);
  Xen_Instance* raw = Xen_String_From_CString(buffer);
  if (!raw) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Dealloc(buffer);
  return raw;
}

static Xen_Instance* string_hash(Xen_INSTANCE* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  unsigned long hash = Xen_String_Hash(self);
  Xen_INSTANCE* hash_inst = Xen_Number_From_ULong(hash);
  if (!hash_inst) {
    return NULL;
  }
  return hash_inst;
}

static Xen_Instance* string_boolean(Xen_INSTANCE* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (((Xen_String*)self)->characters) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* string_opr_eq(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->string)
    return NULL;

  Xen_Instance* val = Xen_Tuple_Get_Index(args, 0);
  if (strcmp(Xen_String_As_CString(self), Xen_String_As_CString(val)) == 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* string_opr_ne(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->string)
    return NULL;

  Xen_Instance* val = Xen_Tuple_Get_Index(args, 0);
  if (strcmp(Xen_String_As_CString(self), Xen_String_As_CString(val)) != 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* string_opr_get_index(Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* index_inst = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(index_inst) != xen_globals->implements->number)
    return NULL;
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) {
    return NULL;
  }
  Xen_Instance* character =
      Xen_String_From_Char(((Xen_String*)self)->characters[index]);
  if (!character) {
    return NULL;
  }
  return character;
}

static Xen_Instance* string_opr_add(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* str = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(str) != xen_globals->implements->string)
    return NULL;
  return Xen_String_From_Concat(self, str);
}

static Xen_Instance* string_opr_mul(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* num_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(num_inst) != xen_globals->implements->number)
    return NULL;
  size_t num = Xen_Number_As(Xen_size_t, num_inst);
  Xen_size_t bufcap = Xen_SIZE(self) * num + 1;
  char* buffer = Xen_Alloc(bufcap);
  if (!buffer) {
    return NULL;
  }
  const char* value = Xen_String_As_CString(self);
  buffer[0] = '\0';
  for (Xen_size_t i = 0; i < num; i++) {
    strcat(buffer, value);
  }
  buffer[bufcap - 1] = '\0';
  Xen_Instance* string = Xen_String_From_CString(buffer);
  if (!string) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Dealloc(buffer);
  return string;
}

static Xen_Instance* string_opr_not(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(self) == 0) {
    return Xen_False;
  }
  return Xen_True;
}

static Xen_Instance* string_prop_upper(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  char* buffer = Xen_Alloc(Xen_SIZE(self) + 1);
  if (!buffer) {
    return NULL;
  }
  for (Xen_size_t i = 0; i < Xen_SIZE(self); i++) {
    buffer[i] = toupper(((Xen_String*)self)->characters[i]);
  }
  buffer[Xen_SIZE(self)] = '\0';
  Xen_Instance* result = Xen_String_From_CString(buffer);
  if (!result) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Dealloc(buffer);
  return result;
}

static Xen_Instance* string_prop_lower(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  char* buffer = Xen_Alloc(Xen_SIZE(self) + 1);
  if (!buffer) {
    return NULL;
  }
  for (Xen_size_t i = 0; i < Xen_SIZE(self); i++) {
    buffer[i] = tolower(((Xen_String*)self)->characters[i]);
  }
  buffer[Xen_SIZE(self)] = '\0';
  Xen_Instance* result = Xen_String_From_CString(buffer);
  if (!result) {
    Xen_Dealloc(buffer);
    return NULL;
  }
  Xen_Dealloc(buffer);
  return result;
}

static Xen_Instance* string_char_code(Xen_Instance* self, Xen_Instance* args,
                                      Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* result = Xen_Number_From_Int(Xen_String_As_Char(self));
  if (!result) {
    return NULL;
  }
  return result;
}

static Xen_Instance* string_bytes(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Xen_String* string = (Xen_String*)self;
  Xen_Instance* bytes =
      Xen_Bytes_From_Array(Xen_SIZE(string), (Xen_uint8_t*)string->characters);
  Xen_Bytes_Append(bytes, 0);
  return bytes;
}

static Xen_Instance* string_split(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  Xen_String* string = (Xen_String*)self;
  Xen_Function_ArgSpec args_def[] = {
      {"delimiters", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_TUPLE, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* delimiters =
      Xen_Function_ArgBinding_Search(binding, "delimiters")->value;
  Xen_Function_ArgBinding_Free(binding);
  Xen_Instance* tokens = Xen_Vector_New();
  Xen_size_t start = 0;
  Xen_size_t i = 0;
  for (; i < string->__size; i++) {
    for (Xen_size_t y = 0; y < Xen_SIZE(delimiters); y++) {
      Xen_Instance* delim_inst = Xen_Tuple_Get_Index(delimiters, y);
      if (!Xen_IsString(delim_inst)) {
        return NULL;
      }
      Xen_String* delim = (Xen_String*)delim_inst;
      if (strncmp(string->characters + i, delim->characters, delim->__size) ==
          0) {
        if (i != start) {
          Xen_string_t token_str = Xen_ZAlloc(i - start + 1, 1);
          strncpy(token_str, string->characters + start, i - start);
          Xen_Instance* token = Xen_String_From_CString(token_str);
          Xen_Vector_Push(tokens, token);
          free(token_str);
        }
        i += delim->__size;
        start = i;
        break;
      }
    }
  }
  if (start != i) {
    Xen_Instance* token = Xen_String_From_CString(string->characters + start);
    Xen_Vector_Push(tokens, token);
  }
  return tokens;
}

static Xen_Instance* string_split_once(Xen_Instance* self, Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  Xen_String* string = (Xen_String*)self;
  Xen_Function_ArgSpec args_def[] = {
      {"delimiter", XEN_FUNCTION_ARG_KIND_POSITIONAL,
       XEN_FUNCTION_ARG_IMPL_STRING, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };
  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_String* delimiter =
      (Xen_String*)Xen_Function_ArgBinding_Search(binding, "delimiter")->value;
  Xen_Function_ArgBinding_Free(binding);
  Xen_Instance* tokens = Xen_Vector_New();
  for (Xen_size_t i = 0; i + delimiter->__size <= string->__size; i++) {
    if (strncmp(string->characters + i, delimiter->characters,
                delimiter->__size) == 0) {
      Xen_string_t lhs_str = Xen_ZAlloc(i + 1, 1);
      strncpy(lhs_str, string->characters, i);
      Xen_Instance* lhs = Xen_String_From_CString(lhs_str);
      Xen_Vector_Push(tokens, lhs);
      free(lhs_str);
      Xen_string_t rhs_str =
          Xen_ZAlloc(string->__size - (i + delimiter->__size) + 1, 1);
      strncpy(rhs_str, string->characters + i + delimiter->__size,
              string->__size - (i + delimiter->__size));
      Xen_Instance* rhs = Xen_String_From_CString(rhs_str);
      Xen_Vector_Push(tokens, rhs);
      free(rhs_str);
      return tokens;
    }
  }
  Xen_Vector_Push(tokens, self);
  Xen_Vector_Push(tokens, Xen_String_From_CString(""));
  return tokens;
}

static Xen_Instance* string_trim(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Xen_String* string = (Xen_String*)self;
  Xen_size_t start = 0;
  Xen_size_t end = string->__size;
  while (isspace(string->characters[start]) && start < end) {
    start++;
  }
  while (isspace(string->characters[end - 1]) && end > start) {
    end--;
  }
  Xen_string_t result_str = Xen_ZAlloc(end - start + 1, 1);
  strncpy(result_str, string->characters + start, end - start);
  Xen_Instance* result = Xen_String_From_CString(result_str);
  return result;
}

static struct __Implement __String_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "String",
    .__inst_size = sizeof(struct Xen_String_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = string_alloc,
    .__create = NULL,
    .__destroy = string_destroy,
    .__string = string_string,
    .__raw = string_raw,
    .__callable = NULL,
    .__hash = string_hash,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

struct __Implement* Xen_String_GetImplement(void) {
  return &__String_Implement;
}

int Xen_String_Init(void) {
  if (!Xen_VM_Store_Global("string",
                           (Xen_Instance*)xen_globals->implements->string)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__eq", string_opr_eq, nil) ||
      !Xen_VM_Store_Native_Function(props, "__ne", string_opr_ne, nil) ||
      !Xen_VM_Store_Native_Function(props, "__get_index", string_opr_get_index,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__add", string_opr_add, nil) ||
      !Xen_VM_Store_Native_Function(props, "__mul", string_opr_mul, nil) ||
      !Xen_VM_Store_Native_Function(props, "__boolean", string_boolean, nil) ||
      !Xen_VM_Store_Native_Function(props, "__not", string_opr_not, nil) ||
      !Xen_VM_Store_Native_Function(props, "upper", string_prop_upper, nil) ||
      !Xen_VM_Store_Native_Function(props, "lower", string_prop_lower, nil) ||
      !Xen_VM_Store_Native_Function(props, "char_code", string_char_code,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "bytes", string_bytes, nil) ||
      !Xen_VM_Store_Native_Function(props, "split", string_split, nil) ||
      !Xen_VM_Store_Native_Function(props, "split_once", string_split_once,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "trim", string_trim, nil)) {
    return 0;
  }
  __String_Implement.__props = props;
  Xen_IGC_Fork_Push(impls_maps, props);
  return 1;
}

void Xen_String_Finish(void) {}
