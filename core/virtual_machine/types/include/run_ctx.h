#ifndef __RUN_CTX_H__
#define __RUN_CTX_H__

#include <stdbool.h>
#include <stdint.h>

#include "call_args.h"
#include "callable.h"
#include "instance.h"
#include "vm_register.h"

struct RunContext {
  struct RunContext *ctx_caller;
  struct __Instance *ctx_self;
  CALLABLE_ptr ctx_code;
  VM_Register ctx_reg;
  CallArgs *ctx_args;
  uint32_t ctx_ip;
  bool ctx_running;
};

typedef struct RunContext *RunContext_ptr;

RunContext_ptr run_context_new(RunContext_ptr, struct __Instance *, CallArgs *);
void run_context_free(const RunContext_ptr);

#endif
