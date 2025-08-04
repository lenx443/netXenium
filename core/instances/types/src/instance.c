#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "call_args.h"
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
  CallArgs *alloc_args = call_args_new();
  if (alloc_args) {
    free(inst->__name);
    free(inst);
    return NULL;
  }
  if (!call_args_push(alloc_args, (struct CArg){
                                      CARG_POINTER,
                                      &inst->__pointer,
                                      sizeof(void *),
                                  })) {
    call_args_free(alloc_args);
    free(inst->__name);
    free(inst);
    return NULL;
  }
  if (!call_args_push(alloc_args, (struct CArg){
                                      CARG_POINTER,
                                      &inst->__pointer_size,
                                      sizeof(size_t),
                                  })) {
    call_args_free(alloc_args);
    free(inst->__name);
    free(inst);
    return NULL;
  }
  if (!vm_run_callable(impl->__alloc, inst, alloc_args)) {
    call_args_free(alloc_args);
    free(inst->__name);
    free(inst);
    return NULL;
  }
  return inst;
}

void __instnace_free(struct __Instance *inst) {
  if (!inst) { return; }
  CallArgs *destroy_args = call_args_new();
  if (!destroy_args) {
    free(inst->__pointer);
    free(inst->__name);
    free(inst);
    return;
  }
  if (!call_args_push(destroy_args, (struct CArg){
                                        CARG_POINTER,
                                        inst->__pointer,
                                        inst->__pointer_size,
                                    })) {
    call_args_free(destroy_args);
    free(inst->__pointer);
    free(inst->__name);
    free(inst);
    return;
  }
  if (!vm_run_callable(inst->__impl->__destroy, inst, destroy_args)) {
    call_args_free(destroy_args);
    free(inst->__pointer);
    free(inst->__name);
    free(inst);
    return;
  }
  free(inst->__name);
  free(inst);
}
