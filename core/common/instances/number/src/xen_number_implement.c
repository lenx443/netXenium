#include <iso646.h>
#include <stdint.h>

#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "instance_life.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_boolean_instance.h"
#include "xen_bytes.h"
#include "xen_function.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static Xen_Instance* number_alloc(Xen_INSTANCE* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* val = Xen_Tuple_Get_Index(args, 0);
    Xen_Instance* rsult = NULL;
    if (Xen_IMPL(val) == xen_globals->implements->number) {
      rsult = val;
    } else if (Xen_IMPL(val) == xen_globals->implements->string) {
      const char* str = Xen_String_As_CString(val);
      int base = 0;
      if (kwargs && Xen_IMPL(kwargs) == xen_globals->implements->map) {
        Xen_Instance* base_inst = Xen_Map_Get_Str(kwargs, "base");
        if (base_inst) {
          if (Xen_IMPL(base_inst) != xen_globals->implements->number) {
            return NULL;
          }
          base = Xen_Number_As_Int(base_inst);
        }
      }
      rsult = Xen_Number_From_CString(str, base);
    } else if (Xen_IMPL(val) == xen_globals->implements->boolean) {
      rsult = Xen_Number_From_Int(((Xen_Boolean*)val)->value);
    }
    return rsult;
  }
  Xen_Number* num =
      (Xen_Number*)Xen_Instance_Alloc(xen_globals->implements->number);
  if (!num) {
    return NULL;
  }
  num->digits = NULL;
  num->scale = 0;
  num->size = 0;
  num->sign = 0;
  return (Xen_Instance*)num;
}

static Xen_Instance* number_create(Xen_INSTANCE* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Number* n = (Xen_Number*)self;
  if (!n->digits) {
    n->digits = Xen_Alloc(sizeof(uint32_t));
    if (!n->digits) {
      return NULL;
    }
    n->digits[0] = 0;
    n->scale = 0;
    n->size = 1;
    n->sign = 0;
  }
  return nil;
}

static Xen_Instance* number_destroy(Xen_INSTANCE* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Number* num = (Xen_Number*)self;
  if (num->digits)
    Xen_Dealloc(num->digits);
  return nil;
}

static Xen_Instance* number_string(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  const char* cstring = Xen_Number_As_CString(self);
  if (!cstring) {
    return NULL;
  }
  Xen_Instance* string = Xen_String_From_CString(cstring);
  if (!string) {
    Xen_Dealloc((void*)cstring);
    return NULL;
  }
  Xen_Dealloc((void*)cstring);
  return string;
}

static Xen_Instance* number_hash(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;

  Xen_Number* n = (Xen_Number*)self;
  int64_t hash = 0;
  int64_t y;
  Xen_ssize_t i = n->size;

  const int SHIFT = 32;
  const int HASH_BITS = 8 * sizeof(int64_t);

  while (--i >= 0) {
    y = (int64_t)n->digits[i];
    hash = ((hash << SHIFT) | (hash >> (HASH_BITS - SHIFT))) ^ y;
  }

  hash ^= n->size;
  if (n->sign < 0)
    hash = -hash;

  return Xen_Number_From_Int64(hash);
}

static Xen_Instance* number_boolean(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (((Xen_Number*)self)->sign == 0) {
    return Xen_False;
  }
  return Xen_True;
}

static Xen_Instance* number_opr_pow(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* exp = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* result = Xen_Number_Pow(self, exp);
  if (!result) {
    return NULL;
  }
  return result;
}

static Xen_Instance* number_opr_mul(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* num = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* result = Xen_Number_Mul(self, num);
  if (!result) {
    return NULL;
  }
  return result;
}

static Xen_Instance* number_opr_div(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* num = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* result = Xen_Number_Div(self, num);
  if (!result) {
    return NULL;
  }
  return result;
}

static Xen_Instance* number_opr_mod(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* num = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* result = Xen_Number_Mod(self, num);
  if (!result) {
    return NULL;
  }
  return result;
}

static Xen_Instance* number_opr_add(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* num = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* result = Xen_Number_Add(self, num);
  if (!result) {
    return NULL;
  }
  return result;
}

static Xen_Instance* number_opr_sub(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* num = Xen_Tuple_Get_Index(args, 0);
  Xen_Instance* result = Xen_Number_Sub(self, num);
  if (!result) {
    return NULL;
  }
  return result;
}

static Xen_Instance* number_opr_eq(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Number_Cmp(a, b) == 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* number_opr_ne(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Number_Cmp(a, b) != 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* number_opr_lt(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Number_Cmp(a, b) < 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* number_opr_le(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Number_Cmp(a, b) <= 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* number_opr_gt(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Number_Cmp(a, b) > 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* number_opr_ge(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) !=
          xen_globals->implements->number)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Number_Cmp(a, b) >= 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* number_prop_positive(Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return self;
}

static Xen_Instance* number_prop_negative(Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Number* n = (Xen_Number*)self;
  Xen_size_t size_n = n->size;
  while (size_n > 0 && n->digits[size_n - 1] == 0) {
    size_n--;
  }
  if (size_n == 0 || n->sign == 0) {
    return (Xen_Instance*)n;
  }
  Xen_Number* r =
      (Xen_Number*)__instance_new(xen_globals->implements->number, nil, nil, 0);
  if (!r) {
    return NULL;
  }
  r->digits = Xen_Alloc(size_n * sizeof(uint32_t));
  for (Xen_size_t i = 0; i < size_n; i++) {
    r->digits[i] = n->digits[i];
  }
  r->scale = n->scale;
  r->size = size_n;
  if (n->sign == 1) {
    r->sign = -1;
  } else if (n->sign == -1) {
    r->sign = 1;
  } else {
    return NULL;
  }
  return (Xen_Instance*)r;
}

static Xen_Instance* number_prop_not(Xen_Instance* self, Xen_Instance* args,
                                     Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (((Xen_Number*)self)->sign == 0) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* number_bytes(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  if (!Xen_Function_ArgEmpty(args, kwargs)) {
    return NULL;
  }
  Xen_size_t len;
  uint8_t* arr = Xen_Number_As_Bytes(self, &len);
  Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
  return result;
}

static Xen_Instance* number_i8(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 1, 1, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 1, 1, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}
static Xen_Instance* number_i16(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 2, 1, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 2, 1, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}
static Xen_Instance* number_i32(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 4, 1, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 4, 1, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}

static Xen_Instance* number_i64(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 8, 1, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 8, 1, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}

static Xen_Instance* number_i128(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 16, 1, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 16, 1, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}

static Xen_Instance* number_u8(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 1, 0, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 1, 0, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}
static Xen_Instance* number_u16(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 2, 0, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 2, 0, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}
static Xen_Instance* number_u32(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 4, 0, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 4, 0, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}

static Xen_Instance* number_u64(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 8, 0, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 8, 0, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}

static Xen_Instance* number_u128(Xen_Instance* self, Xen_Instance* args,
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
  if (big_ending == Xen_True) {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 16, 0, 0xff, 1);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  } else {
    Xen_size_t len;
    uint8_t* arr = Xen_Number_As_Bytes_Flexible(self, &len, 16, 0, 0xff, 0);
    Xen_Instance* result = Xen_Bytes_From_Array(len, arr);
    return result;
  }
}

static struct __Implement __Number_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Number",
    .__inst_size = sizeof(struct Xen_Number_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = NULL,
    .__props = NULL,
    .__alloc = number_alloc,
    .__create = number_create,
    .__destroy = number_destroy,
    .__string = number_string,
    .__raw = number_string,
    .__callable = NULL,
    .__hash = number_hash,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

struct __Implement* Xen_Number_GetImplement(void) {
  return &__Number_Implement;
}

int Xen_Number_Init(void) {
  if (!Xen_VM_Store_Global("number",
                           (Xen_Instance*)xen_globals->implements->number)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__boolean", number_boolean, nil) ||
      !Xen_VM_Store_Native_Function(props, "__pow", number_opr_pow, nil) ||
      !Xen_VM_Store_Native_Function(props, "__mul", number_opr_mul, nil) ||
      !Xen_VM_Store_Native_Function(props, "__div", number_opr_div, nil) ||
      !Xen_VM_Store_Native_Function(props, "__mod", number_opr_mod, nil) ||
      !Xen_VM_Store_Native_Function(props, "__add", number_opr_add, nil) ||
      !Xen_VM_Store_Native_Function(props, "__sub", number_opr_sub, nil) ||
      !Xen_VM_Store_Native_Function(props, "__eq", number_opr_eq, nil) ||
      !Xen_VM_Store_Native_Function(props, "__ne", number_opr_ne, nil) ||
      !Xen_VM_Store_Native_Function(props, "__lt", number_opr_lt, nil) ||
      !Xen_VM_Store_Native_Function(props, "__le", number_opr_le, nil) ||
      !Xen_VM_Store_Native_Function(props, "__gt", number_opr_gt, nil) ||
      !Xen_VM_Store_Native_Function(props, "__ge", number_opr_ge, nil) ||
      !Xen_VM_Store_Native_Function(props, "__positive", number_prop_positive,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__negative", number_prop_negative,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__not", number_prop_not, nil) ||
      !Xen_VM_Store_Native_Function(props, "bytes", number_bytes, nil) ||
      !Xen_VM_Store_Native_Function(props, "i8", number_i8, nil) ||
      !Xen_VM_Store_Native_Function(props, "i16", number_i16, nil) ||
      !Xen_VM_Store_Native_Function(props, "i32", number_i32, nil) ||
      !Xen_VM_Store_Native_Function(props, "i64", number_i64, nil) ||
      !Xen_VM_Store_Native_Function(props, "i128", number_i128, nil) ||
      !Xen_VM_Store_Native_Function(props, "u8", number_u8, nil) ||
      !Xen_VM_Store_Native_Function(props, "u16", number_u16, nil) ||
      !Xen_VM_Store_Native_Function(props, "u32", number_u32, nil) ||
      !Xen_VM_Store_Native_Function(props, "u64", number_u64, nil) ||
      !Xen_VM_Store_Native_Function(props, "u128", number_u128, nil)) {
    return 0;
  }
  __Number_Implement.__props = props;
  Xen_IGC_Fork_Push(impls_maps, props);
  return 1;
}

void Xen_Number_Finish(void) {}
