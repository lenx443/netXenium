#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "instance.h"
#include "instances_array.h"

struct __Instances_Array *__instances_array_new() {
  struct __Instances_Array *inst_array = malloc(sizeof(struct __Instances_Array));
  if (!inst_array) { return NULL; }
  inst_array->__inst = NULL;
  inst_array->__size = 0;
  inst_array->__capacity = 0;
  return inst_array;
}

bool __instances_array_push(struct __Instances_Array *inst_array,
                            struct __Instance *inst) {
  if (!inst_array || !inst) { return false; }
  if (inst_array->__size < inst_array->__capacity) {
    size_t capacity_new = inst_array->__capacity == 0 ? 4 : inst_array->__capacity * 2;
    struct __Instance **new_mem =
        realloc(inst_array->__inst, sizeof(struct __Instance *) * capacity_new);
    if (!new_mem) { return false; }
    inst_array->__inst = new_mem;
    inst_array->__capacity = capacity_new;
  }
  inst_array->__inst[inst_array->__size++] = inst;
  return true;
}

void __instances_array_free(struct __Instances_Array *inst_array) {
  if (!inst_array) { return; }
  for (int i = 0; i < inst_array->__size; i++) {
    __instnace_free(inst_array->__inst[i]);
  }
  free(inst_array->__inst);
  free(inst_array);
}
