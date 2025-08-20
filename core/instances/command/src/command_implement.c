#include "basic.h"
#include "call_args.h"
#include "callable.h"
#include "command_implement.h"
#include "command_instance.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_register.h"

static int command_alloc(ctx_id_t id, struct __Instance *self, CallArgs *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  inst->cmd_callable = NULL;
  return 1;
}

static int command_destroy(ctx_id_t id, struct __Instance *self, CallArgs *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  if (inst->cmd_callable) callable_free(inst->cmd_callable);
  return 1;
}

static int command_callable(ctx_id_t id, struct __Instance *self, CallArgs *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  Xen_INSTANCE *self_caller = xen_register_prop_get("__expose", id);
  if (inst->cmd_callable) {
    if (vm_run_callable(inst->cmd_callable, self_caller, args)) { return 0; }
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
