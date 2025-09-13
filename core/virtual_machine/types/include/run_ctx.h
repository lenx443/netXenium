#ifndef __RUN_CTX_H__
#define __RUN_CTX_H__

#include <stdbool.h>
#include <stdint.h>

#include "callable.h"
#include "instance.h"
#include "vm_register.h"

#define CTX_FLAG_STATIC (1 << 0)
#define CTX_FLAG_PROPS (1 << 1)

#define CTX_GET_FLAG(ctx, flag) (((((struct RunContext *)ctx)->ctx_flags) & (flag)) != 0)

typedef unsigned long ctx_id_t;

struct RunContext {
  Xen_INSTANCE_HEAD;
  uint8_t ctx_flags;
  ctx_id_t ctx_id;
  Xen_Instance *ctx_closure;
  Xen_Instance *ctx_caller;
  Xen_Instance *ctx_self;
  CALLABLE_ptr ctx_code;
  VM_Register ctx_reg;
  Xen_Instance *ctx_args;
  struct __Instance *ctx_instances;
  uint32_t ctx_ip;
  bool ctx_running;
};

ctx_id_t run_ctx_id(Xen_Instance *);

typedef struct RunContext *RunContext_ptr;

#endif
