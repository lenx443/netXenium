#include "xen_tuple_iterator_implement.h"
#include "attrs.h"
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
#include "xen_tuple_iterator_instance.h"
#include "xen_typedefs.h"

static Xen_Instance* tuple_iterator_alloc(ctx_id_t id, Xen_Instance* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple_Iterator* it =
      (Xen_Tuple_Iterator*)Xen_Instance_Alloc(&Xen_Tuple_Iterator_Implement);
  if (!it) {
    return NULL;
  }
  it->tuple = NULL;
  it->index = -1;
  return (Xen_Instance*)it;
}

static Xen_Instance* tuple_iterator_destroy(ctx_id_t id, Xen_Instance* self,
                                            Xen_Instance* args,
                                            Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple_Iterator* it = (Xen_Tuple_Iterator*)self;
  if (it->tuple) {
    Xen_DEL_REF(it->tuple);
  }
  return nil;
}

static Xen_Instance* tuple_iterator_iter(ctx_id_t id, Xen_Instance* self,
                                         Xen_Instance* args,
                                         Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_ADD_REF(self);
}

static Xen_Instance* tuple_iterator_next(ctx_id_t id, Xen_Instance* self,
                                         Xen_Instance* args,
                                         Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Tuple_Iterator* it = (Xen_Tuple_Iterator*)self;
  if (it->index == -1) {
    return NULL;
  }
  if ((Xen_size_t)it->index >= Xen_SIZE(it->tuple)) {
    it->index = -1;
    return NULL;
  }
  Xen_Instance* rsult = Xen_Attr_Index_Size_Get(it->tuple, it->index++);
  if (!rsult) {
    return NULL;
  }
  return rsult;
}

Xen_Implement Xen_Tuple_Iterator_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "TupleIterator",
    .__inst_size = sizeof(struct Xen_Tuple_Iterator_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = tuple_iterator_alloc,
    .__create = NULL,
    .__destroy = tuple_iterator_destroy,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = &Xen_Basic_Get_Attr_Static,
    .__set_attr = NULL,
};

int Xen_Tuple_Iterator_Init() {
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__iter", tuple_iterator_iter,
                                    nil) ||
      !Xen_VM_Store_Native_Function(props, "__next", tuple_iterator_next,
                                    nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Tuple_Iterator_Implement.__props = props;
  return 1;
}

void Xen_Tuple_Iterator_Finish() {
  Xen_DEL_REF(Xen_Tuple_Iterator_Implement.__props);
}
