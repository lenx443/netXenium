#include <stdlib.h>

#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"

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

static int string_hash(ctx_id_t id, Xen_INSTANCE *self, Xen_Instance *args) {
  if (!VM_CHECK_ID(id)) { return 0; }
  Xen_String *string = (Xen_String *)self;
  unsigned long hash = 0x1505;
  int c;
  while ((c = *string->characters++))
    hash = ((hash << 5) + hash) + c;
  Xen_INSTANCE *hash_inst = Xen_Number_From_ULong(hash);
  if_nil_eval(hash_inst) { return 0; }
  if (!xen_register_prop_set("__expose_hash", hash_inst, id)) {
    Xen_DEL_REF(hash_inst);
    return 0;
  }
  return 1;
}

struct __Implement Xen_String_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "String",
    .__inst_size = sizeof(struct Xen_String_Instance),
    .__inst_default_flags = XEN_INSTANCE_FLAG_MAPPED,
    .__props = NULL,
    .__alloc = string_alloc,
    .__destroy = string_destroy,
    .__callable = NULL,
    .__hash = string_hash,
};
