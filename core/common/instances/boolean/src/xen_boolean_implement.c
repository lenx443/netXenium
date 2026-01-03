#include "xen_boolean_implement.h"
#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "instance_life.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_boolean_instance.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"

static Xen_Instance* boolean_alloc(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* inst = Xen_Tuple_Get_Index(args, 0);
    Xen_Instance* boolean = Xen_Attr_Boolean(inst);
    if (!boolean) {
      return NULL;
    }
    return boolean;
  }
  return Xen_False;
}

static Xen_Instance* boolean_string(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Boolean* boolean = (Xen_Boolean*)self;
  Xen_Instance* string = NULL;
  if (boolean->value == 0) {
    if ((string = Xen_String_From_CString("false")) == NULL) {
      return NULL;
    }
  } else if (boolean->value == 1) {
    if ((string = Xen_String_From_CString("true")) == NULL) {
      return NULL;
    }
  } else {
    if ((string = Xen_String_From_CString("unknow")) == NULL) {
      return NULL;
    }
  }
  return string;
}

static Xen_Instance* boolean_hash(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  unsigned long hash = (unsigned long)((Xen_Boolean*)self)->value;
  Xen_Instance* hash_number = Xen_Number_From_ULong(hash);
  if (!hash_number) {
    return NULL;
  }
  return hash_number;
}

static Xen_Instance* boolean_boolean(Xen_Instance* self, Xen_Instance* args,
                                     Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return self;
}

static Xen_Instance* boolean_not(Xen_Instance* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Boolean* boolean = (Xen_Boolean*)self;
  if (!boolean->value) {
    return Xen_True;
  }
  return Xen_False;
}

Xen_Implement __Boolean_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Boolean",
    .__inst_size = sizeof(struct Xen_Boolean_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = boolean_alloc,
    .__create = NULL,
    .__destroy = NULL,
    .__string = boolean_string,
    .__raw = boolean_string,
    .__callable = NULL,
    .__hash = boolean_hash,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

struct __Implement* Xen_Boolean_GetImplement(void) {
  return &__Boolean_Implement;
}

int Xen_Boolean_Init(void) {
  if (!Xen_VM_Store_Global("boolean",
                           (Xen_Instance*)xen_globals->implements->boolean) ||
      !Xen_VM_Store_Global("true", Xen_True) ||
      !Xen_VM_Store_Global("false", Xen_False)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__boolean", boolean_boolean, nil) ||
      !Xen_VM_Store_Native_Function(props, "__not", boolean_not, nil)) {
    return 0;
  }
  __Boolean_Implement.__props = Xen_GCHandle_New_From((Xen_GCHeader*)props);
  Xen_IGC_Fork_Push(impls_maps, props);
  return 1;
}

void Xen_Boolean_Finish(void) {
  Xen_GCHandle_Free(__Boolean_Implement.__props);
}
