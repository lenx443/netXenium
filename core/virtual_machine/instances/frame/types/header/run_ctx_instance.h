#ifndef __RUN_CTX_INSTANCE_H__
#define __RUN_CTX_INSTANCE_H__

#include <stdbool.h>
#include <stdint.h>

#include "gc_header.h"
#include "instance.h"
#include "vm_catch_stack.h"

struct RunContext {
  Xen_INSTANCE_HEAD
  struct VM_Catch_Stack* ctx_catch_stack;
  Xen_GCHandle* ctx_closure;
  Xen_GCHandle* ctx_caller;
  Xen_GCHandle* ctx_self;
  Xen_GCHandle* ctx_code;
  Xen_GCHandle* ctx_stack;
  Xen_GCHandle* ctx_args;
  Xen_GCHandle* ctx_kwargs;
  Xen_GCHandle* ctx_globals;
  Xen_GCHandle* ctx_instances;
  Xen_GCHandle* ctx_scopes;
  Xen_ulong_t ctx_ip;
  Xen_uint8_t ctx_flags;
  bool ctx_running;
  bool ctx_error;
};

typedef struct RunContext* RunContext_ptr;

#endif
