#ifndef __VM_DEF_H__
#define __VM_DEF_H__

#include <stdbool.h>

#include "run_ctx.h"
#include "run_ctx_stack.h"

typedef struct {
  RunContext_Stack_ptr vm_ctx_stack;
  RunContext_ptr root_context;
  Xen_Instance *modules_contexts;
  Xen_Instance *global_props;
  ctx_id_t ctx_id_count;
} VM;

typedef VM *VM_ptr;

bool vm_create();
void vm_destroy();

extern VM_ptr vm;

#endif
