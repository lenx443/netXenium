#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "implement.h"
#include "instance.h"

struct __Instance *__instance_new(struct __Implement *impl) {
  if (!impl) { return NULL; }
  struct __Instance *inst = malloc(impl->__inst_size);
  if (!inst) { return NULL; }
  inst->__refers = 1;
  inst->__impl = impl;
  if (!impl->__alloc(inst, NULL)) {
    free(inst);
    return NULL;
  }
  inst->__flags = 0x00;
  return inst;
}

void __instance_free(struct __Instance *inst) {
  if (!inst) { return; }
  if (!XEN_INSTANCE_FLAG_IS_STATIC(inst->__flags)) {
    inst->__impl->__destroy(inst, NULL);
    free(inst);
  }
}
