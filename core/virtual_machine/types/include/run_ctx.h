#ifndef __RUN_CTX_H__
#define __RUN_CTX_H__

#include <stdbool.h>
#include <stdint.h>

#include "program_code.h"
#include "vm_register.h"

typedef struct {
  ProgramCode_t ctx_code;
  VM_Register ctx_reg;
  uint32_t ctx_ip;
  bool ctx_running;
} RunContext;

typedef RunContext *RunContext_ptr;

RunContext_ptr run_context_new();
void run_context_free(const RunContext_ptr);

#endif
