#include "run_ctx.h"
#include "callable.h"
#include "instance.h"
#include "run_ctx_instance.h"
#include "vm_scope.h"
#include "vm_stack.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_nil.h"

Xen_Instance* Xen_Ctx_New(Xen_Instance* caller, Xen_Instance* closure, Xen_Instance* self,
                          Xen_Instance* args, Xen_Instance* kwargs, Xen_Instance* globals,
                          Xen_Instance* instances, Xen_VM_Scopes* scopes, CALLABLE_ptr code) {
  RunContext_ptr ctx = (RunContext_ptr)__instance_new(
      xen_globals->implements->run_frame, nil, nil, 0);
  if (!ctx) {
    return NULL;
  }
  Xen_IGC_Push((Xen_Instance*)ctx);
  ctx->ctx_catch_stack = NULL;
  ctx->ctx_ip = 0;
  ctx->ctx_running = 0;
  ctx->ctx_error = 0;
  if (!caller || Xen_Nil_Eval(caller)) {
    ctx->ctx_caller->ptr = NULL;
  } else {
    Xen_IGC_WRITE_FIELD(ctx->ctx_caller, caller);
  }
  if (!closure || Xen_Nil_Eval(closure)) {
    Xen_GC_Write_Field(&ctx->ctx_closure, (Xen_GCHeader*)nil);
  } else {
    Xen_IGC_WRITE_FIELD(ctx->ctx_closure, closure);
  }
  if (!self) {
    Xen_GC_Write_Field(&ctx->ctx_self, (Xen_GCHeader*)nil);
  } else {
    Xen_GC_Write_Field(&ctx->ctx_self, (Xen_GCHeader*)self);
  }
  Xen_IGC_WRITE_FIELD(ctx->ctx_args, args);
  Xen_IGC_WRITE_FIELD(ctx->ctx_kwargs, kwargs);
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
  Xen_IGC_WRITE_FIELD(ctx->ctx_instances, instances);
  if (globals) {
    if (Xen_IMPL(globals) != xen_globals->implements->map) {
      Xen_IGC_Pop();
      return NULL;
    }
    Xen_IGC_WRITE_FIELD(ctx->ctx_globals, globals);
  }
  if (scopes) {
    Xen_GC_Write_Field(&ctx->ctx_scopes, (struct __GC_Header*)scopes);
  } else {
    Xen_GC_Write_Field(&ctx->ctx_scopes, (struct __GC_Header*)Xen_VM_Scopes_New()
    );
  }
  if (code) {
    Xen_GC_Write_Field(&ctx->ctx_code, (Xen_GCHeader*)code);
  }
  Xen_IGC_WRITE_FIELD(ctx->ctx_stack, vm_stack_new(code->code.stack_depth + 1));
  Xen_IGC_Pop();
  return (Xen_Instance*)ctx;
}

void Xen_Ctx_Enable_End(Xen_Instance*ctx_inst) {
  ((RunContext_ptr)ctx_inst)->ctx_flags |= RUN_CTX_FLAG_END;
}
