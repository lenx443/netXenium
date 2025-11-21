#ifndef __RUN_CTX_INSTANCE_H__
#define __RUN_CTX_INSTANCE_H__

#include <stdbool.h>
#include <stdint.h>

#include "callable.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm_stack.h"

struct RunContext {
  Xen_INSTANCE_HEAD;
  uint8_t ctx_flags;
  ctx_id_t ctx_id;
  Xen_Instance* ctx_closure;
  Xen_Instance* ctx_caller;
  Xen_Instance* ctx_self;
  CALLABLE_ptr ctx_code;
  struct vm_Stack* ctx_stack;
  Xen_Instance* ctx_args;
  Xen_Instance* ctx_kwargs;
  struct __Instance* ctx_instances;
  Xen_ulong_t ctx_ip;
  bool ctx_running;
  bool ctx_error;
};

typedef struct RunContext* RunContext_ptr;

#endif
