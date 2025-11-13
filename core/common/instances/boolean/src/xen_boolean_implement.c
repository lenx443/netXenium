#include "xen_boolean_implement.h"
#include "attrs.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_boolean_instance.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"

static Xen_Instance* boolean_alloc(ctx_id_t id, Xen_Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) > 1) {
    return NULL;
  } else if (Xen_SIZE(args) == 1) {
    Xen_Instance* inst = Xen_Attr_Index_Size_Get(args, 0);
    Xen_Instance* boolean = Xen_Attr_Boolean(inst);
    if (!boolean) {
      Xen_DEL_REF(inst);
      return NULL;
    }
    Xen_DEL_REF(inst);
    return boolean;
  }
  return Xen_False;
}

static Xen_Instance* boolean_string(ctx_id_t id, Xen_Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
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

static Xen_Instance* boolean_hash(ctx_id_t id, Xen_Instance* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  unsigned long hash = (unsigned long)((Xen_Boolean*)self)->value;
  Xen_Instance* hash_number = Xen_Number_From_ULong(hash);
  if (!hash_number) {
    return NULL;
  }
  return hash_number;
}

static Xen_Instance* boolean_boolean(ctx_id_t id, Xen_Instance* self,
                                     Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_ADD_REF(self);
}

Xen_Implement Xen_Boolean_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Boolean",
    .__inst_size = sizeof(struct Xen_Boolean_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = boolean_alloc,
    .__create = NULL,
    .__destroy = NULL,
    .__string = boolean_string,
    .__raw = boolean_string,
    .__callable = NULL,
    .__hash = boolean_hash,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};

int Xen_Boolean_Init() {
  if (!Xen_VM_Store_Global("boolean", (Xen_Instance*)&Xen_Boolean_Implement) ||
      !Xen_VM_Store_Global("true", Xen_True) ||
      !Xen_VM_Store_Global("false", Xen_False)) {
    return 0;
  }
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  if (!Xen_VM_Store_Native_Function(props, "__boolean", boolean_boolean, nil)) {
    Xen_DEL_REF(props);
    return 0;
  }
  Xen_Boolean_Implement.__props = props;
  return 1;
}

void Xen_Boolean_Finish() {
  Xen_DEL_REF(Xen_Boolean_Implement.__props);
}
