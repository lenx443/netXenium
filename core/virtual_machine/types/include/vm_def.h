#ifndef __VM_DEF_H__
#define __VM_DEF_H__

#include <stdbool.h>

#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"

typedef struct {
  RunContext_Stack_ptr vm_ctx_stack;
  Xen_Instance* modules_contexts;
  Xen_Instance* globals_instances;
  Xen_Instance* global_props;
  ctx_id_t ctx_id_count;
} VM;

typedef VM* VM_ptr;

bool vm_create();
void vm_destroy();

extern VM_ptr vm;

#endif
