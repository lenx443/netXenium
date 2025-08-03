#ifndef __RUN_CTX_H__
#define __RUN_CTX_H__

#include <stdbool.h>
#include <stdint.h>

#include "call_args.h"
#include "callable.h"
#include "program_code.h"
#include "vm_register.h"

typedef struct {
  CALLABLE_ptr code;
  VM_Register ctx_reg;
  Unmut_CallArgs *ctx_args;
  uint32_t ctx_ip;
  bool ctx_running;
} RunContext;

typedef RunContext *RunContext_ptr;

RunContext_ptr run_context_new(CallArgs *);
void run_context_free(const RunContext_ptr);

#endif
