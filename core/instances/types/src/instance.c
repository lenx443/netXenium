#include <stdlib.h>

#include "callable.h"
#include "instance.h"

struct __Instance *__instance_new() {
  struct __Instance *inst = malloc(sizeof(struct __Instance));
  if (!inst) { return NULL; }
  inst->__type_index = 0;
  inst->__value_ptr = NULL;
  inst->__as_string = NULL;
  return inst;
}

void __instance_free(struct __Instance *inst) {
  if (!inst) return;
  callable_free(inst->__as_string);
  free(inst->__value_ptr);
  free(inst);
}
