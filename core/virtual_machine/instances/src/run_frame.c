#include "run_frame.h"
#include "basic.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"

static void frame_trace(Xen_GCHeader* h) {
  struct RunContext* ctx = (struct RunContext*)h;
  if_nil_neval(ctx->ctx_closure) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_closure);
  }
  if (ctx->ctx_caller) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_caller);
  }
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_self);
  if (ctx->ctx_code) {
    if (ctx->ctx_code->callable_type == CALL_BYTECODE_PROGRAM) {
      Xen_GC_Trace_GCHeader(
          (Xen_GCHeader*)ctx->ctx_code->code.consts->c_instances);
      Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_code->code.consts->c_names);
    }
  }
  if (ctx->ctx_stack)
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_stack);
  if (ctx->ctx_args) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_args);
  }
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_kwargs);
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)ctx->ctx_instances);
}

static Xen_Instance* frame_alloc(ctx_id_t id, Xen_INSTANCE* self,
                                 Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (!args || Xen_SIZE(args) != 5 ||
      (Xen_Nil_NEval(Xen_Tuple_Peek_Index(args, 0)) &&
       Xen_IMPL(Xen_Tuple_Peek_Index(args, 0)) != &Xen_Run_Frame) ||
      (Xen_Nil_NEval(Xen_Tuple_Peek_Index(args, 1)) &&
       Xen_IMPL(Xen_Tuple_Peek_Index(args, 1)) != &Xen_Run_Frame)) {
    return NULL;
  }
  struct RunContext* ctx_new =
      (struct RunContext*)Xen_Instance_Alloc(&Xen_Run_Frame);
  if (!ctx_new) {
    return NULL;
  }
  ctx_new->ctx_flags = CTX_FLAG_PROPS;
  ctx_new->ctx_id = 0;
  if (!Xen_Tuple_Peek_Index(args, 0))
    ctx_new->ctx_closure = nil;
  else
    Xen_GC_Write_Field((Xen_GCHeader*)ctx_new,
                       (Xen_GCHeader**)&ctx_new->ctx_closure,
                       (Xen_GCHeader*)Xen_Tuple_Get_Index(args, 0));
  if (!Xen_Tuple_Peek_Index(args, 1))
    ctx_new->ctx_caller = NULL;
  else
    Xen_GC_Write_Field((Xen_GCHeader*)ctx_new,
                       (Xen_GCHeader**)&ctx_new->ctx_caller,
                       (Xen_GCHeader*)Xen_Tuple_Get_Index(args, 1));
  Xen_GC_Write_Field((Xen_GCHeader*)ctx_new, (Xen_GCHeader**)&ctx_new->ctx_self,
                     (Xen_GCHeader*)Xen_Tuple_Get_Index(args, 2));
  ctx_new->ctx_code = NULL;
  ctx_new->ctx_stack = NULL;
  if (Xen_Tuple_Peek_Index(args, 3)) {
    Xen_GC_Write_Field((Xen_GCHeader*)ctx_new,
                       (Xen_GCHeader**)&ctx_new->ctx_args,
                       (Xen_GCHeader*)Xen_Tuple_Get_Index(args, 3));
  } else
    ctx_new->ctx_args = NULL;
  if (Xen_Tuple_Peek_Index(args, 4)) {
    Xen_GC_Write_Field((Xen_GCHeader*)ctx_new,
                       (Xen_GCHeader**)&ctx_new->ctx_kwargs,
                       (Xen_GCHeader*)Xen_Tuple_Get_Index(args, 4));
  } else
    ctx_new->ctx_kwargs = NULL;
  ctx_new->ctx_instances = Xen_Map_New();
  Xen_GC_Write_Field((Xen_GCHeader*)ctx_new,
                     (Xen_GCHeader**)&ctx_new->ctx_instances,
                     (Xen_GCHeader*)Xen_Map_New());
  if (!ctx_new->ctx_instances) {
    return NULL;
  }
  ctx_new->ctx_ip = 0;
  ctx_new->ctx_running = 0;
  ctx_new->ctx_error = 0;
  return (Xen_Instance*)ctx_new;
}

static Xen_Instance* frame_destroy(ctx_id_t id, Xen_INSTANCE* self,
                                   Xen_INSTANCE* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return nil;
}

static Xen_Instance* frame_string(ctx_id_t id, Xen_Instance* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<Context-Frame>");
  if (!string) {
    return NULL;
  }
  return string;
}

struct __Implement Xen_Run_Frame = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "RunFrame",
    .__inst_size = sizeof(struct RunContext),
    .__inst_default_flags = 0x00,
    .__inst_trace = frame_trace,
    .__props = &Xen_Nil_Def,
    .__alloc = frame_alloc,
    .__create = NULL,
    .__destroy = frame_destroy,
    .__string = frame_string,
    .__raw = frame_string,
    .__callable = NULL,
    .__hash = NULL,
};
