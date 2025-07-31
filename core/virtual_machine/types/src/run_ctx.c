#include <stdlib.h>

#include "bytecode.h"
#include "run_ctx.h"
#include "vm_register.h"
#include "vm_string_table.h"

RunContext_ptr run_context_new() {
  RunContext_ptr ctx_new = malloc(sizeof(RunContext));
  if (!ctx_new) { return NULL; }
  ctx_new->ctx_code.code = bc_new();
  if (!ctx_new->ctx_code.code) {
    free(ctx_new);
    return NULL;
  }
  ctx_new->ctx_code.strings = vm_string_table_new();
  if (!ctx_new->ctx_code.strings) {
    bc_free(ctx_new->ctx_code.code);
    free(ctx_new);
    return NULL;
  }
  if (!vm_register_new(&ctx_new->ctx_reg)) {
    vm_string_table_free(ctx_new->ctx_code.strings);
    bc_free(ctx_new->ctx_code.code);
    free(ctx_new);
  }
  ctx_new->ctx_ip = 0;
  ctx_new->ctx_running = 0;
  return ctx_new;
}

void run_context_free(const RunContext_ptr ctx) {
  if (!ctx) return;
  vm_register_free(ctx->ctx_reg);
  vm_string_table_free(ctx->ctx_code.strings);
  bc_free(ctx->ctx_code.code);
  free(ctx);
}
