#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "run_frame.h"

ctx_id_t run_ctx_id(Xen_Instance* ctx) {
  if (!ctx || ctx->__impl != &Xen_Run_Frame) {
    return 0;
  }
  return ((RunContext_ptr)ctx)->ctx_id;
}
