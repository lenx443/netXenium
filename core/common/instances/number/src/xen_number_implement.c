#include <iso646.h>
#include <stdint.h>
#include <stdlib.h>

#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_number_instance.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static Xen_Instance* number_alloc(ctx_id_t id, Xen_INSTANCE* self,
                                  Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Number* num = (Xen_Number*)self;
  num->digits = NULL;
  num->size = 0;
  num->sign = 0;
  return nil;
}

static Xen_Instance* number_destroy(ctx_id_t id, Xen_INSTANCE* self,
                                    Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Number* num = (Xen_Number*)self;
  if (num->digits)
    free(num->digits);
  return nil;
}

static Xen_Instance* number_string(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE
  const char* cstring = Xen_Number_As_CString(self);
  if (!cstring) {
    return NULL;
  }
  Xen_Instance* string = Xen_String_From_CString(cstring);
  if (!string) {
    free((void*)cstring);
    return NULL;
  }
  free((void*)cstring);
  return string;
}

static Xen_Instance* number_opr_eq(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_TYPE(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Number* a = (Xen_Number*)self;
  Xen_Number* b = (Xen_Number*)Xen_Operator_Eval_Pair_Steal2(
      args, Xen_Number_From_Int(0), Xen_OPR_GET_INDEX);

  size_t size_a = a->size;
  while (size_a > 0 && a->digits[size_a - 1] == 0) {
    size_a--;
  }

  size_t size_b = b->size;
  while (size_b > 0 && b->digits[size_b - 1] == 0) {
    size_b--;
  }

  if (size_a == 0 && size_b == 0) {
    Xen_DEL_REF(b);
    return Xen_True;
  }

  if (a->sign != b->sign) {
    Xen_DEL_REF(b);
    return Xen_False;
  }

  if (size_a != size_b) {
    Xen_DEL_REF(b);
    return Xen_False;
  }

  for (size_t i = 0; i < size_a; i++) {
    if (a->digits[i] != b->digits[i]) {
      Xen_DEL_REF(b);
      return Xen_False;
    }
  }

  Xen_DEL_REF(b);
  return Xen_True;
}
static Xen_Instance* number_opr_pow(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_TYPE(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* exp = Xen_Operator_Eval_Pair_Steal2(
      args, Xen_Number_From_Int(0), Xen_OPR_GET_INDEX);
  Xen_Instance* result = Xen_Number_Pow(self, exp);
  if (!result) {
    Xen_DEL_REF(exp);
    return NULL;
  }
  Xen_DEL_REF(exp);
  return result;
}

static Xen_Instance* number_opr_mul(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_TYPE(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* num = Xen_Operator_Eval_Pair_Steal2(
      args, Xen_Number_From_Int(0), Xen_OPR_GET_INDEX);
  Xen_Instance* result = Xen_Number_Mul(self, num);
  if (!result) {
    Xen_DEL_REF(num);
    return NULL;
  }
  Xen_DEL_REF(num);
  return result;
}

static Xen_Instance* number_opr_div(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_TYPE(Xen_Vector_Peek_Index(args, 0)) != &Xen_Number_Implement)
    return NULL;

  Xen_Instance* num = Xen_Operator_Eval_Pair_Steal2(
      args, Xen_Number_From_Int(0), Xen_OPR_GET_INDEX);
  Xen_Instance* result = Xen_Number_Div(self, num);
  if (!result) {
    Xen_DEL_REF(num);
    return NULL;
  }
  Xen_DEL_REF(num);
  return result;
}

static Xen_Instance* number_prop_positive(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_ADD_REF(self);
}

static Xen_Instance* number_prop_negative(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Number* n = (Xen_Number*)self;
  Xen_size_t size_n = n->size;
  while (size_n > 0 && n->digits[size_n - 1] == 0) {
    size_n--;
  }
  if (size_n == 0 || n->sign == 0) {
    return (Xen_Instance*)Xen_ADD_REF(n);
  }
  Xen_Number* r = (Xen_Number*)__instance_new(&Xen_Number_Implement, nil, 0);
  if (!r) {
    return NULL;
  }
  r->digits = malloc(size_n * sizeof(uint32_t));
  if (!r->digits) {
    Xen_DEL_REF(r->digits);
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
                                     Xen_Instance* args) {
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
    .__destroy = number_destroy,
    .__string = number_string,
    .__raw = number_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

int Xen_Number_Init() {
  Xen_Instance* props = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if (!props) {
    return 0;
  }
  if (!vm_define_native_function(props, "__pow", number_opr_pow, nil) ||
      !vm_define_native_function(props, "__mul", number_opr_mul, nil) ||
      !vm_define_native_function(props, "__div", number_opr_div, nil) ||
      !vm_define_native_function(props, "__eq", number_opr_eq, nil) ||
      !vm_define_native_function(props, "__positive", number_prop_positive,
                                 nil) ||
      !vm_define_native_function(props, "__negative", number_prop_negative,
                                 nil) ||
      !vm_define_native_function(props, "__not", number_prop_not, nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Number_Implement.__props = props;
  return 1;
}

void Xen_Number_Finish() {
  Xen_DEL_REF(Xen_Number_Implement.__props);
}
