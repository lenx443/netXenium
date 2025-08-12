#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "implement.h"
#include "instance.h"
#include "run_ctx_stack.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_register.h"

struct __Instance *__instance_new(struct __Implement *impl) {
  if (!impl) { return NULL; }
  struct __Instance *inst = malloc(sizeof(struct __Instance));
  if (!inst) { return NULL; }
  inst->__refers = 1;
  inst->__impl = impl;
  if (!vm_run_callable(impl->__alloc, inst, NULL)) {
    free(inst);
    return NULL;
  }
  return inst;
}

void __instance_free(struct __Instance *inst) {
  if (!inst) { return; }
  reg_t reg[32];
  uint8_t reg_flags[32];
  memset(reg, 0, sizeof(reg));
  memset(reg_flags, 0, sizeof(reg_flags));
  struct RunContext static_ctx = {
      .ctx_caller = run_context_stack_peek_top(&vm->vm_ctx_stack),
      .ctx_self = inst,
      .ctx_code = inst->__impl->__destroy,
      .ctx_reg =
          {
              .reg = reg,
              .point_flag = reg_flags,
              .capacity = 32,
          },
      .ctx_args = NULL,
      .ctx_ip = 0,
      .ctx_running = false,
  };
  vm_run_ctx(&static_ctx);
  free(inst);
}
