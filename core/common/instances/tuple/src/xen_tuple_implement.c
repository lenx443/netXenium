#include <stddef.h>
#include <stdlib.h>

#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_number_implement.h"
#include "xen_string.h"
#include "xen_tuple_implement.h"
#include "xen_tuple_instance.h"
#include "xen_vector.h"

static Xen_Instance* tuple_alloc(ctx_id_t id, Xen_Instance* self,
                                 Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple* tuple = (Xen_Tuple*)self;
  tuple->instances = NULL;
  return nil;
}

static Xen_Instance* tuple_destroy(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple* tuple = (Xen_Tuple*)self;
  for (size_t i = 0; i < Xen_SIZE(tuple); i++) {
    Xen_DEL_REF(tuple->instances[i]);
  }
  free(tuple->instances);
  return nil;
}

static Xen_Instance* tuple_string(ctx_id_t id, Xen_Instance* self,
                                  Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<Tuple>");
  if (!string) {
    return NULL;
  }
  return string;
}

static Xen_Instance* tuple_opr_get_index(ctx_id_t id, Xen_Instance* self,
                                         Xen_Instance* args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* index_inst = Xen_Vector_Peek_Index(args, 0);
  if (Xen_IMPL(index_inst) != &Xen_Number_Implement) {
    return NULL;
  }
  size_t index = Xen_Number_As(size_t, index_inst);
  if (index >= self->__size) {
    return NULL;
  }
  return Xen_ADD_REF(((Xen_Tuple*)self)->instances[index]);
}

Xen_Implement Xen_Tuple_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, 0x00),
    .__impl_name = "tuple",
    .__inst_size = sizeof(struct Xen_Tuple_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = tuple_alloc,
    .__destroy = tuple_destroy,
    .__string = tuple_string,
    .__raw = tuple_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

int Xen_Tuple_Init() {
  Xen_Instance* props = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if (!props) {
    return 0;
  }
  if (!vm_define_native_function(props, "__get_index", tuple_opr_get_index,
                                 nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Tuple_Implement.__props = props;
  return 1;
}

void Xen_Tuple_Finish() {
  Xen_DEL_REF(Xen_Tuple_Implement.__props);
}
