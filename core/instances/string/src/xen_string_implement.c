#include <stdlib.h>

#include "basic.h"
#include "call_args.h"
#include "implement.h"
#include "instance.h"
#include "instances_map.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_string_instance.h"

static int string_alloc(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  Xen_String *string = (Xen_String *)self;
  string->characters = NULL;
  string->length = 0;
  string->capacity = 0;
  return 1;
}

static int string_destroy(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  Xen_String *string = (Xen_String *)self;
  free(string->characters);
  return 1;
}

static int string_hash(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  if (!VM_CHECK_ID(id)) { return 0; }
  Xen_String *string = (Xen_String *)self;
  unsigned long hash = 0x1505;
  int c;
  while ((c = *string->characters++))
    hash = ((hash << 5) + hash) + c;
#warning "String Hash in development"
#warning                                                                                 \
    "Integer type required. Please, use the register `__expose` to return the Integer Instanse"
  return 1;
}

static int string_clear(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  Xen_String *string = (Xen_String *)self;
  free(string->characters);
  string->length = 0;
  string->capacity = 0;
  return 1;
}

static int string_push(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  if ((args->size != 1) || (!args->args[0]->pointer) ||
      (args->args[0]->point_type != CARG_INSTANCE) ||
      (Xen_TYPE(args->args[0]->pointer) == &Xen_String_Implement)) {
    return 0;
  }
  Xen_String_Push_CString((Xen_String *)self,
                          ((Xen_String *)args->args[0]->pointer)->characters);
  return 1;
}

int xen_string_implement_init() {
  Xen_String_Implement.__props = __instances_map_new(INSTANCES_MAP_DEFAULT_CAPACITY);
  if (!vm_define_native_command(Xen_String_Implement.__props, "clear", NULL)) {
    __instances_map_free(Xen_String_Implement.__props);
    return 0;
  }
  if (!vm_define_native_command(Xen_String_Implement.__props, "push", string_push)) {
    __instances_map_free(Xen_String_Implement.__props);
    return 0;
  }
  return 1;
}

struct __Implement Xen_String_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "String",
    .__inst_size = sizeof(struct Xen_String_Instance),
    .__props = NULL,
    .__alloc = string_alloc,
    .__destroy = string_destroy,
    .__callable = NULL,
    .__hash = string_hash,
};
