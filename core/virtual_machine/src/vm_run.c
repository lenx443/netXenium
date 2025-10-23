#include "vm_run.h"
#include "bytecode.h"
#include "instance.h"
#include "logs.h"
#include "program.h"
#include "vm_instructs.h"
#include "xen_nil.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM RUN", msg, ##__VA_ARGS__)

Xen_Instance* vm_run_ctx(RunContext_ptr ctx) {
  if (!ctx || !ctx->ctx_code)
    return NULL;
  if (ctx->ctx_code->callable_type == CALL_NATIVE_FUNCTIIN) {
    Xen_Instance* ret = ctx->ctx_code->native_callable(
        ctx->ctx_id, ctx->ctx_self, ctx->ctx_args);
    return ret;
  } else if (ctx->ctx_code->callable_type == CALL_BYTECODE_PROGRAM) {
    ctx->ctx_running = 1;
    ProgramCode_t pc = ctx->ctx_code->code;
    while (ctx->ctx_running && ctx->ctx_ip < pc.code->bc_size &&
           !program.closed) {
      bc_Instruct_t instr = pc.code->bc_array[ctx->ctx_ip++];
      if (instr.bci_opcode > HALT) {
        ctx->ctx_running = 0;
        break;
      }
    }
    log_show_and_clear(NULL);
    return nil;
  }
  return NULL;
}
