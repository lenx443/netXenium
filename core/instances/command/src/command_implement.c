#include "basic.h"
#include "call_args.h"
#include "callable.h"
#include "command_implement.h"
#include "command_instance.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"

static int command_alloc(struct __Instance *self, CallArgs *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  inst->self = NULL;
  inst->cmd_callable = NULL;
  return 1;
}

static int command_destroy(struct __Instance *self, CallArgs *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  if (inst->cmd_callable) callable_free(inst->cmd_callable);
  Xen_DEL_REF(inst->self);
  return 1;
}

static int command_callable(struct __Instance *self, CallArgs *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  if (inst->cmd_callable) {
    if (vm_run_callable(inst->cmd_callable, inst->self, args)) { return 0; }
  }
  return 1;
}

struct __Implement Xen_Command_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Command",
    .__inst_size = sizeof(Xen_Command),
    .__alloc = command_alloc,
    .__destroy = command_destroy,
    .__callable = command_callable,
    .__hash = NULL,
};
