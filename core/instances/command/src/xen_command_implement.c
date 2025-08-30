#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_command_implement.h"
#include "xen_command_instance.h"
#include "xen_nil.h"

static int command_alloc(ctx_id_t id, struct __Instance *self, Xen_Instance *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  inst->cmd_callable = NULL;
  inst->self = nil;
  inst->closure = nil;
  return 1;
}

static int command_destroy(ctx_id_t id, struct __Instance *self, Xen_Instance *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  if_nil_neval(inst->cmd_callable) callable_free(inst->cmd_callable);
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

struct __Implement Xen_Command_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Command",
    .__inst_size = sizeof(Xen_Command),
    .__inst_default_flags = 0x00,
    .__alloc = command_alloc,
    .__destroy = command_destroy,
    .__callable = command_callable,
    .__hash = NULL,
};
