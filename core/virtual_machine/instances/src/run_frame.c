#include "run_frame.h"
#include "basic.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx_instance.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_string.h"

static void frame_trace(Xen_GCHeader* h) {
  struct RunContext* ctx = (struct RunContext*)h;
  if_nil_neval(ctx->ctx_closure) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_closure->ptr);
  }
  if (ctx->ctx_caller) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_caller->ptr);
  }
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_self->ptr);
  if (ctx->ctx_stack)
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_stack->ptr);
  if (ctx->ctx_args) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_args->ptr);
  }
  if (ctx->ctx_kwargs) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_kwargs->ptr);
  }
  if (ctx->ctx_instances)
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_instances->ptr);
  if (ctx->ctx_code) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_code->ptr);
  }
}

static Xen_Instance* frame_alloc(Xen_INSTANCE* self, Xen_Instance* args,
                                 Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct RunContext* ctx_new = (struct RunContext*)Xen_Instance_Alloc(
      xen_globals->implements->run_frame);
  if (!ctx_new) {
    return NULL;
  }
  Xen_IGC_Push((Xen_Instance*)ctx_new);
  ctx_new->ctx_flags = 0;
  ctx_new->ctx_id = 0;
  ctx_new->ctx_catch_stack = NULL;
  ctx_new->ctx_closure = Xen_GCHandle_New_From((Xen_GCHeader*)nil);
  ctx_new->ctx_caller = Xen_GCHandle_New();
  ctx_new->ctx_self = Xen_GCHandle_New_From((Xen_GCHeader*)nil);
  ctx_new->ctx_code = Xen_GCHandle_New();
  ctx_new->ctx_stack = Xen_GCHandle_New();
  ctx_new->ctx_args = Xen_GCHandle_New();
  ctx_new->ctx_kwargs = Xen_GCHandle_New();
  ctx_new->ctx_instances = Xen_GCHandle_New();
  ctx_new->ctx_ip = 0;
  ctx_new->ctx_running = 0;
  ctx_new->ctx_error = 0;
  Xen_IGC_Pop();
  return (Xen_Instance*)ctx_new;
}

static Xen_Instance* frame_destroy(Xen_INSTANCE* self, Xen_INSTANCE* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct RunContext* ctx = (struct RunContext*)self;
  Xen_GCHandle_Free(ctx->ctx_closure);
  Xen_GCHandle_Free(ctx->ctx_caller);
  Xen_GCHandle_Free(ctx->ctx_self);
  Xen_GCHandle_Free(ctx->ctx_code);
  Xen_GCHandle_Free(ctx->ctx_stack);
  Xen_GCHandle_Free(ctx->ctx_args);
  Xen_GCHandle_Free(ctx->ctx_kwargs);
  Xen_GCHandle_Free(ctx->ctx_instances);
  return nil;
}

static Xen_Instance* frame_string(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<Context-Frame>");
  if (!string) {
    return NULL;
  }
  return string;
}

struct __Implement __Run_Frame = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "RunFrame",
    .__inst_size = sizeof(struct RunContext),
    .__inst_default_flags = 0x00,
    .__inst_trace = frame_trace,
    .__props = NULL,
    .__alloc = frame_alloc,
    .__create = NULL,
    .__destroy = frame_destroy,
    .__string = frame_string,
    .__raw = frame_string,
    .__callable = NULL,
    .__hash = NULL,
};

struct __Implement* Xen_Run_Frame_GetImplement(void) {
  return &__Run_Frame;
}
