#include <iso646.h>
#include <stdint.h>

#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_boolean_implement.h"
#include "xen_boolean_instance.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static Xen_Instance* number_alloc(ctx_id_t id, Xen_INSTANCE* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(args, 0);
    Xen_Instance* rsult = NULL;
    if (Xen_IMPL(val) == &Xen_Number_Implement) {
      rsult = Xen_ADD_REF(val);
    } else if (Xen_IMPL(val) == &Xen_String_Implement) {
      const char* str = Xen_String_As_CString(val);
      int base = 0;
      if (kwargs && Xen_IMPL(kwargs) == &Xen_Map_Implement) {
        Xen_Instance* base_inst = Xen_Map_Get_Str(kwargs, "base");
        if (base_inst) {
          if (Xen_IMPL(base_inst) != &Xen_Number_Implement) {
            Xen_DEL_REF(base_inst);
            Xen_DEL_REF(val);
            return NULL;
          }
          base = Xen_Number_As_Int(base_inst);
          Xen_DEL_REF(base_inst);
        }
      }
      rsult = Xen_Number_From_CString(str, base);
    } else if (Xen_IMPL(val) == &Xen_Boolean_Implement) {
      rsult = Xen_Number_From_Int(((Xen_Boolean*)val)->value);
    }
    Xen_DEL_REF(val);
    return rsult;
  }
  Xen_Number* num = (Xen_Number*)Xen_Instance_Alloc(&Xen_Number_Implement);
  if (!num) {
    return NULL;
  }
  num->digits = NULL;
  num->size = 0;
  num->sign = 0;
  return (Xen_Instance*)num;
}

static Xen_Instance* number_create(ctx_id_t id, Xen_INSTANCE* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Number* n = (Xen_Number*)self;
  if (!n->digits) {
    n->digits = Xen_Alloc(sizeof(uint32_t));
    if (!n->digits) {
      return NULL;
    }
    n->digits[0] = 0;
    n->size = 1;
    n->sign = 0;
  }
  return nil;
}

static Xen_Instance* number_destroy(ctx_id_t id, Xen_INSTANCE* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Number* num = (Xen_Number*)self;
  if (num->digits)
    Xen_Dealloc(num->digits);
  return nil;
}

static Xen_Instance* number_string(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
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

static Xen_Instance* number_hash(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
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

static Xen_Instance* number_boolean(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (((Xen_Number*)self)->sign == 0) {
    return Xen_False;
  }
  return Xen_True;
}

static Xen_Instance* number_opr_pow(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* exp = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* result = Xen_Number_Pow(self, exp);
  if (!result) {
    Xen_DEL_REF(exp);
    return NULL;
  }
  Xen_DEL_REF(exp);
  return result;
}

static Xen_Instance* number_opr_mul(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* num = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* result = Xen_Number_Mul(self, num);
  if (!result) {
    Xen_DEL_REF(num);
    return NULL;
  }
  Xen_DEL_REF(num);
  return result;
}

static Xen_Instance* number_opr_div(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* num = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* result = Xen_Number_Div(self, num);
  if (!result) {
    Xen_DEL_REF(num);
    return NULL;
  }
  Xen_DEL_REF(num);
  return result;
}

static Xen_Instance* number_opr_mod(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* num = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* result = Xen_Number_Mod(self, num);
  if (!result) {
    Xen_DEL_REF(num);
    return NULL;
  }
  Xen_DEL_REF(num);
  return result;
}

static Xen_Instance* number_opr_add(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* num = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* result = Xen_Number_Add(self, num);
  if (!result) {
    Xen_DEL_REF(num);
    return NULL;
  }
  Xen_DEL_REF(num);
  return result;
}

static Xen_Instance* number_opr_sub(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* num = Xen_Attr_Index_Size_Get(args, 0);
  Xen_Instance* result = Xen_Number_Sub(self, num);
  if (!result) {
    Xen_DEL_REF(num);
    return NULL;
  }
  Xen_DEL_REF(num);
  return result;
}

static Xen_Instance* number_opr_eq(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Attr_Index_Size_Get(args, 0);
  if (Xen_Number_Cmp(a, b) == 0) {
    Xen_DEL_REF(b);
    return Xen_True;
  }
  Xen_DEL_REF(b);
  return Xen_False;
}

static Xen_Instance* number_opr_ne(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Attr_Index_Size_Get(args, 0);
  if (Xen_Number_Cmp(a, b) != 0) {
    Xen_DEL_REF(b);
    return Xen_True;
  }
  Xen_DEL_REF(b);
  return Xen_False;
}

static Xen_Instance* number_opr_lt(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Attr_Index_Size_Get(args, 0);
  if (Xen_Number_Cmp(a, b) < 0) {
    Xen_DEL_REF(b);
    return Xen_True;
  }
  Xen_DEL_REF(b);
  return Xen_False;
}

static Xen_Instance* number_opr_le(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Attr_Index_Size_Get(args, 0);
  if (Xen_Number_Cmp(a, b) <= 0) {
    Xen_DEL_REF(b);
    return Xen_True;
  }
  Xen_DEL_REF(b);
  return Xen_False;
}

static Xen_Instance* number_opr_gt(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Attr_Index_Size_Get(args, 0);
  if (Xen_Number_Cmp(a, b) > 0) {
    Xen_DEL_REF(b);
    return Xen_True;
  }
  Xen_DEL_REF(b);
  return Xen_False;
}

static Xen_Instance* number_opr_ge(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* a = self;
  Xen_Instance* b = Xen_Attr_Index_Size_Get(args, 0);
  if (Xen_Number_Cmp(a, b) >= 0) {
    Xen_DEL_REF(b);
    return Xen_True;
  }
  Xen_DEL_REF(b);
  return Xen_False;
}

static Xen_Instance* number_prop_positive(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_ADD_REF(self);
}

static Xen_Instance* number_prop_negative(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Number* n = (Xen_Number*)self;
  Xen_size_t size_n = n->size;
  while (size_n > 0 && n->digits[size_n - 1] == 0) {
    size_n--;
  }
  if (size_n == 0 || n->sign == 0) {
    return (Xen_Instance*)Xen_ADD_REF(n);
  }
  Xen_Number* r =
      (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, nil, 0);
  if (!r) {
    return NULL;
  }
  r->digits = Xen_Alloc(size_n * sizeof(uint32_t));
  if (!r->digits) {
    Xen_DEL_REF(r);
    return NULL;
  }
  for (Xen_size_t i = 0; i < size_n; i++) {
    r->digits[i] = n->digits[i];
  }
  r->size = size_n;
  if (n->sign == 1) {
    r->sign = -1;
  } else if (n->sign == -1) {
    r->sign = 1;
  } else {
    Xen_DEL_REF(r);
    return NULL;
  }
  return (Xen_Instance*)r;
}

static Xen_Instance* number_prop_not(ctx_id_t id, Xen_Instance* self,
                                     Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (((Xen_Number*)self)->sign == 0) {
    return Xen_True;
  }
  return Xen_False;
}

struct __Implement Xen_Number_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Number",
    .__inst_size = sizeof(struct Xen_Number_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = number_alloc,
    .__create = number_create,
    .__destroy = number_destroy,
    .__string = number_string,
    .__raw = number_string,
    .__callable = NULL,
    .__hash = number_hash,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

int Xen_Number_Init() {
  if (!Xen_VM_Store_Global("number", (Xen_Instance*)&Xen_Number_Implement)) {
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
      !Xen_VM_Store_Native_Function(props, "__not", number_prop_not, nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Number_Implement.__props = props;
  return 1;
}

void Xen_Number_Finish() {
  Xen_DEL_REF(Xen_Number_Implement.__props);
}
