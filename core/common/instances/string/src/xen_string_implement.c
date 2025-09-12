#include <stdlib.h>

#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"
#include "xen_vector.h"

static int string_alloc(ctx_id_t id, Xen_INSTANCE *self, Xen_Instance *args) {
  Xen_String *string = (Xen_String *)self;
  string->characters = NULL;
  string->length = 0;
  return 1;
}

static int string_destroy(ctx_id_t id, Xen_INSTANCE *self, Xen_Instance *args) {
  Xen_String *string = (Xen_String *)self;
  free(string->characters);
  return 1;
}

static int string_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  if (!xen_register_prop_set("__expose_string", self, id)) { return 0; }
  return 1;
}

static int string_hash(ctx_id_t id, Xen_INSTANCE *self, Xen_Instance *args) {
  if (!VM_CHECK_ID(id)) { return 0; }
  Xen_String *string = (Xen_String *)self;
  char *temp = string->characters;
  unsigned long hash = 0x1505;
  int c;
  while ((c = *temp++))
    hash = ((hash << 5) + hash) + c;
  Xen_INSTANCE *hash_inst = Xen_Number_From_ULong(hash);
  if_nil_eval(hash_inst) { return 0; }
  if (!xen_register_prop_set("__expose_hash", hash_inst, id)) {
    Xen_DEL_REF(hash_inst);
    return 0;
  }
  Xen_DEL_REF(hash_inst);
  return 1;
}

static int string_opr_eq(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  if (Xen_Nil_Eval(args) || Xen_Vector_Size(args) < 1 ||
      Xen_TYPE(Xen_Vector_Peek_Index(args, 0)) != &Xen_String_Implement)
    return 0;

  Xen_Instance *val = Xen_Vector_Get_Index(args, 0);
  if (strcmp(Xen_String_As_CString(self), Xen_String_As_CString(val)) == 0) {
    Xen_DEL_REF(val);
    if (!xen_register_prop_set("__expose_opr_eq", Xen_True, id)) { return 0; }
    return 1;
  }
  Xen_DEL_REF(val);
  if (!xen_register_prop_set("__expose_opr_eq", Xen_False, id)) { return 0; }
  return 1;
}

struct __Implement Xen_String_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "String",
    .__inst_size = sizeof(struct Xen_String_Instance),
    .__inst_default_flags = XEN_INSTANCE_FLAG_MAPPED,
    .__props = NULL,
    .__opr = {NULL},
    .__alloc = string_alloc,
    .__destroy = string_destroy,
    .__string = string_string,
    .__raw = string_string,
    .__callable = NULL,
    .__hash = string_hash,
};

int Xen_String_Init() {
  Xen_String_Implement.__opr.__eq = callable_new_native(string_opr_eq);
  return 1;
}

void Xen_String_Finish() {}
