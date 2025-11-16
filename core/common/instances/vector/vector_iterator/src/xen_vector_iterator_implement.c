#include "xen_vector_iterator_implement.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_vector.h"
#include "xen_vector_iterator_instance.h"

static Xen_Instance* vector_iterator_alloc(ctx_id_t id, Xen_Instance* self,
                                           Xen_Instance* args,
                                           Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector_Iterator* it =
      (Xen_Vector_Iterator*)Xen_Instance_Alloc(&Xen_Vector_Iterator_Implement);
  if (!it) {
    return NULL;
  }
  it->vector = NULL;
  it->index = -1;
  return (Xen_Instance*)it;
}

static Xen_Instance* vector_iterator_destroy(ctx_id_t id, Xen_Instance* self,
                                             Xen_Instance* args,
                                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector_Iterator* it = (Xen_Vector_Iterator*)self;
  if (it->vector) {
    Xen_DEL_REF(it->vector);
  }
  return nil;
}

static Xen_Instance* vector_iterator_iter(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_ADD_REF(self);
}

static Xen_Instance* vector_iterator_next(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Vector_Iterator* it = (Xen_Vector_Iterator*)self;
  if (it->index == -1) {
    return NULL;
  }
  if ((Xen_size_t)it->index >= Xen_SIZE(it->vector)) {
    it->index = -1;
    return NULL;
  }
  Xen_Instance* rsult = Xen_Vector_Get_Index(it->vector, it->index++);
  if (!rsult) {
    return NULL;
  }
  return rsult;
}

Xen_Implement Xen_Vector_Iterator_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "VectorIterator",
    .__inst_size = sizeof(struct Xen_Vector_Iterator_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = vector_iterator_alloc,
    .__create = NULL,
    .__destroy = vector_iterator_destroy,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
    .__set_attr = NULL,
};

int Xen_Vector_Iterator_Init() {
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__iter", vector_iterator_iter,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__next", vector_iterator_next,
                                    nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Vector_Iterator_Implement.__props = props;
  return 1;
}

void Xen_Vector_Iterator_Finish() {
  Xen_DEL_REF(Xen_Vector_Iterator_Implement.__props);
}
