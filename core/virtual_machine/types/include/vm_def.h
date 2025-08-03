#ifndef __VM_DEF_H__
#define __VM_DEF_H__

#include "run_ctx.h"
#include "run_ctx_stack.h"

typedef struct {
  RunContext_Stack_ptr vm_ctx_stack;
  RunContext_ptr root_context;
} VM;

typedef VM *VM_ptr;

int vm_create();
void vm_destroy();

extern VM_ptr vm;

#endif
