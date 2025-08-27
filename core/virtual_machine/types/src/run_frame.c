#include "basic.h"
#include "call_args.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_frame.h"

static int frame_alloc(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  if (!args || args->size != 3 || args->args[0]->point_type != CARG_INSTANCE ||
      args->args[1]->point_type != CARG_INSTANCE ||
      args->args[2]->point_type != CARG_POINTER ||
      (args->args[0]->pointer != NULL &&
       Xen_TYPE(args->args[0]->pointer) != &Xen_Run_Frame)) {
    return 0;
  }
  struct RunContext *ctx_new = (struct RunContext *)self;
  ctx_new->ctx_flags = CTX_FLAG_PROPS;
  ctx_new->ctx_id = 0;
  ctx_new->ctx_caller = (struct RunContext *)args->args[0]->pointer;
  ctx_new->ctx_self = (Xen_INSTANCE *)args->args[1]->pointer;
  ctx_new->ctx_code = NULL;
  if (!vm_register_new(&ctx_new->ctx_reg)) { return 0; }
  if (args->args[2]->pointer) {
    ctx_new->ctx_args = (CallArgs *)args->args[2]->pointer;
    if (!ctx_new->ctx_args) { return 0; }
  } else
    ctx_new->ctx_args = NULL;
  ctx_new->ctx_instances = __instances_map_new(INSTANCES_MAP_DEFAULT_CAPACITY);
  if (!ctx_new->ctx_instances) {
    if (ctx_new->ctx_args) call_args_free(ctx_new->ctx_args);
    vm_register_free(ctx_new->ctx_reg);
    return 0;
  }
  ctx_new->ctx_ip = 0;
  ctx_new->ctx_running = 0;
  return 1;
}

static int frame_destroy(ctx_id_t id, Xen_INSTANCE *self, CallArgs *args) {
  struct RunContext *ctx = (struct RunContext *)self;
  __instances_map_free(ctx->ctx_instances);
  if (ctx->ctx_args) call_args_free(ctx->ctx_args);
  vm_register_free(ctx->ctx_reg);
  return 1;
}

struct __Implement Xen_Run_Frame = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "RunFrame",
    .__inst_size = sizeof(struct RunContext),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = frame_alloc,
    .__destroy = frame_destroy,
    .__callable = NULL,
    .__hash = NULL,
};
