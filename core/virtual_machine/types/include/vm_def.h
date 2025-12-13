#ifndef __VM_DEF_H__
#define __VM_DEF_H__

#include <stdbool.h>

#include "gc_header.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "vm_backtrace.h"
#include "xen_typedefs.h"

typedef struct {
  Xen_GCHeader gc;
  RunContext_Stack_ptr vm_ctx_stack;
  Xen_Instance* modules;
  Xen_Instance* modules_stack;
  Xen_Instance* globals_instances;
  Xen_Instance* globals_props;
  ctx_id_t ctx_id_count;
  Xen_c_string_t path_current;
  Xen_Instance* paths_modules;
  struct {
    Xen_bool_t active;
    Xen_Instance* except;
    vm_backtrace* bt;
  } except;
} VM;

typedef VM* VM_ptr;

bool vm_create(void);
void vm_destroy(void);

extern VM_ptr vm;

#endif
