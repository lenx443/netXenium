#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

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
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

static Xen_Instance* string_alloc(ctx_id_t id, Xen_INSTANCE* self,
                                  Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_String* string = (Xen_String*)self;
  string->characters = NULL;
  return nil;
}

static Xen_Instance* string_destroy(ctx_id_t id, Xen_INSTANCE* self,
                                    Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_String* string = (Xen_String*)self;
  if (string->characters)
    free(string->characters);
  return nil;
}

static Xen_Instance* string_string(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_ADD_REF(self);
}

static Xen_Instance* string_raw(ctx_id_t id, Xen_Instance* self,
                                Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_String* string = (Xen_String*)self;
  char* buffer = malloc(strlen(string->characters) + 3);
  if (!buffer) {
    return NULL;
  }
  strcpy(buffer, "'");
  strcat(buffer, string->characters);
  strcat(buffer, "'");
  Xen_Instance* raw = Xen_String_From_CString(buffer);
  if (!raw) {
    free(buffer);
    return NULL;
  }
  free(buffer);
  return raw;
}

static Xen_Instance* string_hash(ctx_id_t id, Xen_INSTANCE* self,
                                 Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  unsigned long hash = Xen_String_Hash(self);
  Xen_INSTANCE* hash_inst = Xen_Number_From_ULong(hash);
  if (!hash_inst) {
    return NULL;
  }
  return hash_inst;
}

static Xen_Instance* string_opr_eq(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_Nil_Eval(args) || Xen_SIZE(args) < 1 ||
      Xen_IMPL(Xen_Vector_Peek_Index(args, 0)) != &Xen_String_Implement)
    return NULL;

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
  if (Xen_IMPL(index_inst) != &Xen_Number_Implement)
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

static Xen_Instance* string_opr_add(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* str = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(str) != &Xen_String_Implement)
    return NULL;
  return Xen_String_From_Concat(self, str);
}

static Xen_Instance* string_opr_mul(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1)
    return NULL;
  Xen_Instance* num_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(num_inst) != &Xen_Number_Implement)
    return NULL;
  size_t num = Xen_Number_As(Xen_size_t, num_inst);
  Xen_size_t bufcap = Xen_SIZE(self) * num + 1;
  char* buffer = malloc(bufcap);
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
    free(buffer);
    return NULL;
  }
  free(buffer);
  return string;
}

static Xen_Instance* string_prop_upper(ctx_id_t id, Xen_Instance* self,
                                       Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  char* buffer = malloc(Xen_SIZE(self) + 1);
  if (!buffer) {
    return NULL;
  }
  for (Xen_size_t i = 0; i < Xen_SIZE(self); i++) {
    buffer[i] = toupper(((Xen_String*)self)->characters[i]);
  }
  buffer[Xen_SIZE(self)] = '\0';
  Xen_Instance* result = Xen_String_From_CString(buffer);
  if (!result) {
    free(buffer);
    return NULL;
  }
  free(buffer);
  return result;
}

static Xen_Instance* string_prop_lower(ctx_id_t id, Xen_Instance* self,
                                       Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  char* buffer = malloc(Xen_SIZE(self) + 1);
  if (!buffer) {
    return NULL;
  }
  for (Xen_size_t i = 0; i < Xen_SIZE(self); i++) {
    buffer[i] = tolower(((Xen_String*)self)->characters[i]);
  }
  buffer[Xen_SIZE(self)] = '\0';
  Xen_Instance* result = Xen_String_From_CString(buffer);
  if (!result) {
    free(buffer);
    return NULL;
  }
  free(buffer);
  return result;
}

struct __Implement Xen_String_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "String",
    .__inst_size = sizeof(struct Xen_String_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = string_alloc,
    .__destroy = string_destroy,
    .__string = string_string,
    .__raw = string_raw,
    .__callable = NULL,
    .__hash = string_hash,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

int Xen_String_Init() {
  Xen_Instance* props = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if (!props) {
    return 0;
  }
  if (!vm_define_native_function(props, "__eq", string_opr_eq, nil) ||
      !vm_define_native_function(props, "__get_index", string_opr_get_index,
                                 nil) ||
      !vm_define_native_function(props, "__add", string_opr_add, nil) ||
      !vm_define_native_function(props, "__mul", string_opr_mul, nil) ||
      !vm_define_native_function(props, "upper", string_prop_upper, nil) ||
      !vm_define_native_function(props, "lower", string_prop_lower, nil)) {
    Xen_DEL_REF(Xen_String_Implement.__props);
    return 0;
  }
  Xen_String_Implement.__props = props;
  return 1;
}

void Xen_String_Finish() {
  Xen_DEL_REF(Xen_String_Implement.__props);
}
