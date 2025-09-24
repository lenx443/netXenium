#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_frame.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_vector.h"

static int frame_alloc(ctx_id_t id, Xen_INSTANCE *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (!args || Xen_Vector_Size(args) != 4 ||
      (Xen_Nil_NEval(Xen_Vector_Peek_Index(args, 0)) &&
       Xen_TYPE(Xen_Vector_Peek_Index(args, 0)) != &Xen_Run_Frame) ||
      (Xen_Nil_NEval(Xen_Vector_Peek_Index(args, 1)) &&
       Xen_TYPE(Xen_Vector_Peek_Index(args, 1)) != &Xen_Run_Frame)) {
    return 0;
  }
  struct RunContext *ctx_new = (struct RunContext *)self;
  ctx_new->ctx_flags = CTX_FLAG_PROPS;
  ctx_new->ctx_id = 0;
  if_nil_eval(Xen_Vector_Peek_Index(args, 0)) ctx_new->ctx_closure = nil;
  else ctx_new->ctx_closure = Xen_Vector_Get_Index(args, 0);
  if_nil_eval(Xen_Vector_Peek_Index(args, 1)) ctx_new->ctx_caller = NULL;
  else ctx_new->ctx_caller = Xen_Vector_Get_Index(args, 1);
  ctx_new->ctx_self = Xen_Vector_Get_Index(args, 2);
  ctx_new->ctx_code = NULL;
  if (!vm_register_new(&ctx_new->ctx_reg)) {
    Xen_DEL_REF(ctx_new->ctx_closure);
    if (ctx_new->ctx_caller) Xen_DEL_REF(ctx_new->ctx_caller);
    Xen_DEL_REF(ctx_new->ctx_self);
    return 0;
  }
  if_nil_neval(Xen_Vector_Peek_Index(args, 3)) {
    ctx_new->ctx_args = Xen_Vector_Get_Index(args, 3);
  }
  else ctx_new->ctx_args = NULL;
  ctx_new->ctx_instances = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if (!ctx_new->ctx_instances) {
    Xen_DEL_REF(ctx_new->ctx_closure);
    if (ctx_new->ctx_caller) Xen_DEL_REF(ctx_new->ctx_caller);
    Xen_DEL_REF(ctx_new->ctx_self);
    if (ctx_new->ctx_args) Xen_DEL_REF(ctx_new->ctx_args);
    vm_register_free(ctx_new->ctx_reg);
    return 0;
  }
  ctx_new->ctx_ip = 0;
  ctx_new->ctx_running = 0;
  return 1;
}

static int frame_destroy(ctx_id_t id, Xen_INSTANCE *self, Xen_INSTANCE *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct RunContext *ctx = (struct RunContext *)self;
  Xen_DEL_REF(ctx->ctx_instances);
  Xen_DEL_REF(ctx->ctx_closure);
  if (ctx->ctx_caller) Xen_DEL_REF(ctx->ctx_caller);
  Xen_DEL_REF(ctx->ctx_self);
  if (ctx->ctx_args) Xen_DEL_REF(ctx->ctx_args);
  vm_register_free(ctx->ctx_reg);
  return 1;
}

static int frame_string(ctx_id_t id, Xen_Instance *self, Xen_Instance *args) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance *string = Xen_String_From_CString("<Context-Frame>");
  if (!string) { return 0; }
  if (!xen_register_prop_set("__expose_string", string, id)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return 1;
}

struct __Implement Xen_Run_Frame = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "RunFrame",
    .__inst_size = sizeof(struct RunContext),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__opr = {NULL},
    .__alloc = frame_alloc,
    .__destroy = frame_destroy,
    .__string = frame_string,
    .__raw = frame_string,
    .__callable = NULL,
    .__hash = NULL,
};
