#ifndef __RUN_CTX_H__
#define __RUN_CTX_H__

#include <stdbool.h>
#include <stdint.h>

#include "call_args.h"
#include "callable.h"
#include "instance.h"
#include "vm_register.h"

typedef struct {
  struct __Instance *self;
  CALLABLE_ptr code;
  VM_Register ctx_reg;
  Unmut_CallArgs *ctx_args;
  uint32_t ctx_ip;
  bool ctx_running;
} RunContext;

typedef RunContext *RunContext_ptr;

RunContext_ptr run_context_new(struct __Instance *, CallArgs *);
void run_context_free(const RunContext_ptr);

#endif
