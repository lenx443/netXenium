#include <stdlib.h>

#include "call_args.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm_register.h"

RunContext_ptr run_context_new(RunContext_ptr caller, struct __Instance *self,
                               CallArgs *args) {
  RunContext_ptr ctx_new = malloc(sizeof(struct RunContext));
  if (!ctx_new) { return NULL; }
  ctx_new->ctx_caller = caller;
  ctx_new->ctx_self = self;
  ctx_new->ctx_code = NULL;
  if (!vm_register_new(&ctx_new->ctx_reg)) { free(ctx_new); }
  if (args) {
    ctx_new->ctx_args = args;
    if (!ctx_new->ctx_args) { free(ctx_new); }
  } else
    ctx_new->ctx_args = NULL;
  ctx_new->ctx_ip = 0;
  ctx_new->ctx_running = 0;
  return ctx_new;
}

void run_context_free(const RunContext_ptr ctx) {
  if (!ctx) return;
  if (ctx->ctx_args) call_args_free(ctx->ctx_args);
  vm_register_free(ctx->ctx_reg);
  free(ctx);
}
