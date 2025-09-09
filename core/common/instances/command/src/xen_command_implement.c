#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_command_implement.h"
#include "xen_command_instance.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"

static int command_alloc(ctx_id_t id, struct __Instance *self, Xen_Instance *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  inst->cmd_callable = NULL;
  inst->self = nil;
  inst->closure = nil;
  return 1;
}

static int command_destroy(ctx_id_t id, struct __Instance *self, Xen_Instance *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  if (!inst->cmd_callable) callable_free(inst->cmd_callable);
  if_nil_neval(inst->self) Xen_DEL_REF(inst->self);
  if_nil_neval(inst->closure) Xen_DEL_REF(inst->closure);
  return 1;
}

static int command_callable(ctx_id_t id, struct __Instance *self, Xen_Instance *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  if (inst->cmd_callable) {
    if_nil_neval(inst->self) Xen_ADD_REF(inst->self);
    if (vm_run_callable(inst->cmd_callable, inst->closure, inst->self, args)) {
      return 0;
    }
    if_nil_neval(inst->self) Xen_DEL_REF(inst->self);
  }
  return 1;
}
static int command_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  Xen_Instance *string = Xen_String_From_CString("<Command>");
  if (!string) { return 0; }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

struct __Implement Xen_Command_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Command",
    .__inst_size = sizeof(Xen_Command),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__opr = {NULL},
    .__alloc = command_alloc,
    .__destroy = command_destroy,
    .__string = command_string,
    .__raw = command_string,
    .__callable = command_callable,
    .__hash = NULL,
};
