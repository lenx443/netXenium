#include "run_ctx.h"
#include "callable.h"
#include "instance.h"
#include "run_ctx_instance.h"
#include "vm_stack.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_nil.h"

Xen_Instance* Xen_Ctx_New(Xen_Instance* caller, Xen_Instance* closure,
                          Xen_Instance* self, Xen_Instance* args,
                          Xen_Instance* kwargs, Xen_Instance* instances,
                          CALLABLE_ptr code) {
  RunContext_ptr ctx = (RunContext_ptr)__instance_new(
      xen_globals->implements->run_frame, nil, nil, 0);
  if (!ctx) {
    return NULL;
  }
  Xen_IGC_Push((Xen_Instance*)ctx);
  ctx->ctx_flags = CTX_FLAG_PROPS;
  ctx->ctx_id = 0;
  ctx->ctx_catch_stack = NULL;
  ctx->ctx_ip = 0;
  ctx->ctx_running = 0;
  ctx->ctx_error = 0;
  if (!caller || Xen_Nil_Eval(caller)) {
    ctx->ctx_caller->ptr = NULL;
  } else {
    Xen_IGC_WRITE_FIELD(ctx, ctx->ctx_caller, caller);
  }
  if (!closure || Xen_Nil_Eval(closure)) {
    Xen_GC_Write_Field((Xen_GCHeader*)ctx, (Xen_GCHandle**)&ctx->ctx_closure,
                       (Xen_GCHeader*)nil);
  } else {
    Xen_IGC_WRITE_FIELD(ctx, ctx->ctx_closure, closure);
  }
  if (!self) {
    Xen_GC_Write_Field((Xen_GCHeader*)ctx, (Xen_GCHandle**)&ctx->ctx_self,
                       (Xen_GCHeader*)nil);
  } else {
    Xen_GC_Write_Field((Xen_GCHeader*)ctx, (Xen_GCHandle**)&ctx->ctx_self,
                       (Xen_GCHeader*)self);
  }
  Xen_IGC_WRITE_FIELD(ctx, ctx->ctx_args, args);
  Xen_IGC_WRITE_FIELD(ctx, ctx->ctx_kwargs, kwargs);
  if (!instances) {
    instances = Xen_Map_New();
    if (!instances) {
      Xen_IGC_Pop();
      return NULL;
    }
  }
  if (Xen_IMPL(instances) != xen_globals->implements->map) {
    Xen_IGC_Pop();
    return NULL;
  }
  Xen_IGC_WRITE_FIELD(ctx, ctx->ctx_instances, instances);
  if (code) {
    Xen_GC_Write_Field((Xen_GCHeader*)ctx, (Xen_GCHandle**)&ctx->ctx_code,
                       (Xen_GCHeader*)code);
  }
  Xen_IGC_WRITE_FIELD(ctx, ctx->ctx_stack,
                      vm_stack_new(code->code.stack_depth + 1));
  Xen_IGC_Pop();
  return (Xen_Instance*)ctx;
}

ctx_id_t run_ctx_id(Xen_Instance* ctx) {
  if (!ctx || ctx->__impl != xen_globals->implements->run_frame) {
    return 0;
  }
  return ((RunContext_ptr)ctx)->ctx_id;
}
