#ifndef __VM_DEF_H__
#define __VM_DEF_H__

#include <stdbool.h>

#include "gc_header.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "vm_backtrace.h"
#include "xen_typedefs.h"

typedef struct {
  Xen_GCHeader gc;
  RunContext_Stack_ptr vm_ctx_stack;
  Xen_GCHandle* args;
  Xen_GCHandle* modules;
  Xen_GCHandle* modules_stack;
  Xen_GCHandle* globals_instances;
  Xen_GCHandle* globals_props;
  ctx_id_t ctx_id_count;
  Xen_c_string_t path_current;
  Xen_GCHandle* paths_modules;
  Xen_GCHandle* config;
  struct {
    Xen_bool_t active;
    Xen_GCHandle* except;
    vm_backtrace* bt;
  } except;
} VM;

typedef VM* VM_ptr;

bool vm_create(void);
void vm_destroy(void);

#endif
