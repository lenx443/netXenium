#include <malloc.h>
#include <stdlib.h>

#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"
#include "xen_vector.h"

static Xen_Instance* string_alloc(ctx_id_t id, Xen_INSTANCE* self,
                                  Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_String* string = (Xen_String*)self;
  string->characters = NULL;
  return Xen_True;
}

static Xen_Instance* string_destroy(ctx_id_t id, Xen_INSTANCE* self,
                                    Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_String* string = (Xen_String*)self;
  free(string->characters);
  return nil;
}

static Xen_Instance* string_string(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_ADD_REF(self);
}

static Xen_Instance* string_hash(ctx_id_t id, Xen_INSTANCE* self,
                                 Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (!VM_CHECK_ID(id)) {
    return 0;
  }
  unsigned long hash = Xen_String_Hash(self);
  Xen_INSTANCE* hash_inst = Xen_Number_From_ULong(hash);
  if_nil_eval(hash_inst) {
    return nil;
  }
  return hash_inst;
}

static Xen_Instance* string_opr_eq(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_TYPE(Xen_Vector_Peek_Index(args, 0)) != &Xen_String_Implement)
    return nil;

  Xen_Instance* val = Xen_Operator_Eval_Pair_Steal2(
      args, Xen_Number_From_Int(0), Xen_OPR_GET_INDEX);
  if (strcmp(Xen_String_As_CString(self), Xen_String_As_CString(val)) == 0) {
    Xen_DEL_REF(val);
    return Xen_True;
  }
  Xen_DEL_REF(val);
  return Xen_False;
}

static Xen_Instance* string_opr_get_index(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* index_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_TYPE(index_inst) != &Xen_Number_Implement)
    return NULL;
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) {
    return NULL;
  }
  Xen_Instance* character =
      Xen_String_From_Char(((Xen_String*)self)->characters[index]);
  if_nil_eval(character) {
    return NULL;
  }
  return character;
}

struct __Implement Xen_String_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "String",
    .__inst_size = sizeof(struct Xen_String_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__opr = {NULL},
    .__alloc = string_alloc,
    .__destroy = string_destroy,
    .__string = string_string,
    .__raw = string_string,
    .__callable = NULL,
    .__hash = string_hash,
};

int Xen_String_Init() {
  if (!(Xen_String_Implement.__opr[Xen_OPR_EQ] =
            callable_new_native(string_opr_eq))) {
    return 0;
  }
  if (!(Xen_String_Implement.__opr[Xen_OPR_GET_INDEX] =
            callable_new_native(string_opr_get_index))) {
    callable_free(Xen_String_Implement.__opr[Xen_OPR_GET_INDEX]);
    return 0;
  }
  return 1;
}

void Xen_String_Finish() {
  callable_free(Xen_String_Implement.__opr[Xen_OPR_GET_INDEX]);
  callable_free(Xen_String_Implement.__opr[Xen_OPR_EQ]);
}
