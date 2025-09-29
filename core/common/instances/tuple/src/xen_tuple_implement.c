#include <stddef.h>

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
#include "xen_tuple_implement.h"
#include "xen_tuple_instance.h"
#include "xen_vector.h"

static int tuple_alloc(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple *tuple = (Xen_Tuple *)self;
  tuple->instances = NULL;
  return 1;
}

static int tuple_destroy(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple *tuple = (Xen_Tuple *)self;
  for (size_t i = 0; i < Xen_SIZE(tuple); i++) {
    Xen_DEL_REF(tuple->instances[i]);
  }
  free(tuple->instances);
  return 1;
}

static int tuple_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance *string = Xen_String_From_CString("<Tuple>");
  if_nil_eval(string) { return 0; }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

static int tuple_opr_get_index(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) { return 0; }
  Xen_Instance *index_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_TYPE(index_inst) != &Xen_Number_Implement) { return 0; }
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) { return 0; }
  if (!xen_register_prop_set("__expose_opr", ((Xen_Tuple *)self)->instances[index], id)) {
    return 0;
  }
  return 1;
}

Xen_Implement Xen_Tuple_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, 0x00),
    .__impl_name = "tuple",
    .__inst_size = sizeof(struct Xen_Tuple_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__opr = {NULL},
    .__alloc = tuple_alloc,
    .__destroy = tuple_destroy,
    .__string = tuple_string,
    .__raw = tuple_string,
    .__callable = NULL,
    .__hash = NULL,
};

int Xen_Tuple_Init() {
  if (!(Xen_Tuple_Implement.__opr[Xen_OPR_GET_INDEX] =
            callable_new_native(tuple_opr_get_index))) {
    return 0;
  }
  return 1;
}

void Xen_Tuple_Finish() { callable_free(Xen_Tuple_Implement.__opr[Xen_OPR_GET_INDEX]); }
