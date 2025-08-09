#include <stdbool.h>
#include <stdlib.h>

#include "call_args.h"
#include "callable.h"
#include "command_implement.h"
#include "command_instance.h"
#include "implement.h"
#include "implementations.h"
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
  return 1;
}

static int command_callable(struct __Instance *self, CallArgs *args) {
  Xen_Command_ptr inst = (Xen_Command_ptr)self;
  if (inst->cmd_callable) {
    if (vm_run_callable(inst->cmd_callable, inst->self, args)) { return 0; }
  }
  return 1;
}

bool command_implement(struct __Implementations *impls) {
  if (!impls) { return false; }
  struct __Implement *impl = malloc(sizeof(struct __Implement));
  if (!impl) { return false; }
  impl->__type_index = 0;
  impl->__impl_name = strdup("Command");
  if (!impl->__impl_name) {
    free(impl);
    return false;
  }
  impl->__alloc = callable_new_native(command_alloc);
  if (!impl->__alloc) {
    free(impl->__impl_name);
    free(impl);
    return false;
  }
  impl->__destroy = callable_new_native(command_destroy);
  if (!impl->__destroy) {
    callable_free(impl->__alloc);
    free(impl->__impl_name);
    free(impl);
    return false;
  }
  impl->__callable = callable_new_native(command_callable);
  if (!impl->__callable) {
    callable_free(impl->__destroy);
    callable_free(impl->__alloc);
    free(impl->__impl_name);
    free(impl);
    return false;
  }
  if (!__implementations_push(impls, impl)) {
    callable_free(impl->__callable);
    callable_free(impl->__destroy);
    callable_free(impl->__alloc);
    free(impl->__impl_name);
    free(impl);
    return false;
  }
  return true;
}
