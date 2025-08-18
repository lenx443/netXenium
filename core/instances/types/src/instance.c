#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "call_args.h"
#include "implement.h"
#include "instance.h"

struct __Instance *__instance_new(struct __Implement *impl, CallArgs *args) {
  if (!impl) { return NULL; }
  struct __Instance *inst = malloc(impl->__inst_size);
  if (!inst) { return NULL; }
  inst->__refers = 1;
  inst->__impl = impl;
  if (!impl->__alloc(0, inst, args)) {
    free(inst);
    return NULL;
  }
  inst->__flags = 0x00;
  return inst;
}

void __instance_free(struct __Instance *inst) {
  if (!inst) { return; }
  if (!XEN_INSTANCE_GET_FLAG(inst, XEN_INSTANCE_FLAG_STATIC)) {
    inst->__impl->__destroy(0, inst, NULL);
    free(inst);
  }
}
