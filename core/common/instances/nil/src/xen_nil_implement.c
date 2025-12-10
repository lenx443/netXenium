#include "xen_nil_implement.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "instance_life.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"

static Xen_Instance* nil_string(Xen_Instance* self, Xen_Instance* args,
                                Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_String_From_CString("<Nil>");
}

static Xen_Instance* nil_boolean(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_False;
}

static Xen_Instance* nil_eq(Xen_Instance* self, Xen_Instance* args,
                            Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* value = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Nil_Eval(value)) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* nil_ne(Xen_Instance* self, Xen_Instance* args,
                            Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* value = Xen_Tuple_Get_Index(args, 0);
  if (Xen_Nil_NEval(value)) {
    return Xen_True;
  }
  return Xen_False;
}

static Xen_Instance* nil_not(Xen_Instance* self, Xen_Instance* args,
                             Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_False;
}

struct __Implement Xen_Nil_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Nil",
    .__inst_size = sizeof(Xen_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__create = NULL,
    .__destroy = NULL,
    .__string = nil_string,
    .__raw = nil_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

int Xen_Nil_Init(void) {
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__boolean", nil_boolean, nil) ||
      !Xen_VM_Store_Native_Function(props, "__eq", nil_eq, nil) ||
      !Xen_VM_Store_Native_Function(props, "__ne", nil_ne, nil) ||
      !Xen_VM_Store_Native_Function(props, "__not", nil_not, nil)) {
    return 0;
  }
  Xen_IGC_Fork_Push(impls_maps, props);
  Xen_Nil_Implement.__props = props;
  return 1;
}

void Xen_Nil_Finish(void) {}
