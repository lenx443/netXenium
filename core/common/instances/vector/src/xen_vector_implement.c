#include <stddef.h>
#include <stdlib.h>

#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "run_ctx.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"
#include "xen_vector_instance.h"

static int vector_alloc(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector *vector = (Xen_Vector *)self;
  vector->values = NULL;
  vector->capacity = 0;
  return 1;
}

static int vector_destroy(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector *vector = (Xen_Vector *)self;
  for (size_t i = 0; i < vector->__size; i++) {
    Xen_DEL_REF(vector->values[i]);
  }
  free(vector->values);
  return 1;
}

static int vector_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance *string = Xen_String_From_CString("<Vector>");
  if (!string) { return 0; }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

static int vector_opr_get_index(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) return 0;
  Xen_Instance *index_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_TYPE(index_inst) != &Xen_Number_Implement) return 0;
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) { return 0; }
  if (!xen_register_prop_set("__expose_opr", ((Xen_Vector *)self)->values[index], id)) {
    return 0;
  }
  return 1;
}

struct __Implement Xen_Vector_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Vector",
    .__inst_size = sizeof(struct Xen_Vector_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__opr = {NULL},
    .__alloc = vector_alloc,
    .__destroy = vector_destroy,
    .__string = vector_string,
    .__raw = vector_string,
    .__callable = NULL,
    .__hash = NULL,
};

int Xen_Vector_Init() {
  if (!(Xen_Vector_Implement.__opr[Xen_OPR_GET_INDEX] =
            callable_new_native(vector_opr_get_index))) {
    return 0;
  }
  return 1;
}

void Xen_Vector_Finish() { callable_free(Xen_Vector_Implement.__opr[Xen_OPR_GET_INDEX]); }
