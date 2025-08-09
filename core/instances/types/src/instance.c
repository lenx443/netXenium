#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "implement.h"
#include "instance.h"
#include "vm.h"

struct __Instance *__instance_new(char *name, struct __Implement *impl) {
  if (!impl) { return NULL; }
  struct __Instance *inst = malloc(sizeof(struct __Instance));
  if (!inst) { return NULL; }
  inst->__name = strdup(name);
  if (!inst->__name) {
    free(inst);
    return NULL;
  }
  inst->__impl = impl;
  if (!vm_run_callable(impl->__alloc, inst, NULL)) {
    free(inst->__name);
    free(inst);
    return NULL;
  }
  return inst;
}

void __instance_free(struct __Instance *inst) {
  if (!inst) { return; }
  if (!vm_run_callable(inst->__impl->__destroy, inst, NULL)) {
    free(inst->__name);
    free(inst);
    return;
  }
  free(inst->__name);
  free(inst);
}
