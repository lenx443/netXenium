#include <stdlib.h>

#include "call_args.h"
#include "callable.h"
#include "instance.h"
#include "vm.h"

struct __Instance *__instance_new() {
  struct __Instance *inst = malloc(sizeof(struct __Instance));
  if (!inst) { return NULL; }
  inst->__type_index = 0;
  inst->__value_size = 0;
  inst->__value_ptr = NULL;
  inst->__elements_table = NULL;
  inst->__alloc = NULL;
  inst->__destroy = NULL;
  inst->__as_string = NULL;
  return inst;
}

void __instance_free(struct __Instance *inst) {
  if (!inst) return;
  CallArgs *destroy_args = call_args_new();
  callable_free(inst->__alloc);
  if (inst->__destroy) {
    call_args_push(destroy_args, (struct CArg){
                                     CARG_POINTER,
                                     inst->__value_ptr,
                                     inst->__value_size,
                                 });
    vm_run_callable(inst->__destroy, destroy_args);
    callable_free(inst->__destroy);
  }
  callable_free(inst->__as_string);
  free(inst);
}
