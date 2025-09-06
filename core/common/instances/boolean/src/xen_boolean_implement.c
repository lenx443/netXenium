#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_boolean_implement.h"
#include "xen_boolean_instance.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string.h"

static int boolean_alloc(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Boolean *boolean = (Xen_Boolean *)self;
  boolean->value = 0;
  return 1;
}

static int boolean_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Boolean *boolean = (Xen_Boolean *)self;
  Xen_Instance *string = NULL;
  if (boolean->value == 0) {
    if ((string = Xen_String_From_CString("false")) == NULL) { return 0; }
  } else if (boolean->value == 1) {
    if ((string = Xen_String_From_CString("true")) == NULL) { return 0; }
  } else {
    if ((string = Xen_String_From_CString("unknow")) == NULL) { return 0; }
  }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

static int boolean_hash(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  unsigned long hash = (unsigned long)((Xen_Boolean *)self)->value;
  Xen_Instance *hash_number = Xen_Number_From_ULong(hash);
  if (!hash_number) { return 0; }
  if (!xen_register_prop_set("__expose_hash", hash_number, id)) {
    Xen_DEL_REF(hash_number);
  }
  Xen_DEL_REF(hash_number);
  return 1;
}

Xen_Implement Xen_Boolean_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Boolean",
    .__inst_size = sizeof(struct Xen_Boolean_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__opr = {NULL},
    .__alloc = boolean_alloc,
    .__destroy = NULL,
    .__string = boolean_string,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = boolean_hash,
};
