#include <stdint.h>
#include <stdio.h>

#include "callable.h"
#include "compiler.h"
#include "instance.h"
#include "interpreter.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_nil.h"

Xen_Instance* interpreter(const char* text_code, uint8_t compile_mode) {
  if (!text_code) {
    return NULL;
  }
  CALLABLE_ptr code = compiler(text_code, compile_mode);
  if (!code) {
    return NULL;
  }
#ifndef NDEBUG
  printf("== Running ==\n");
#endif
  Xen_Instance* ctx_inst =
      Xen_Ctx_New(nil, nil, nil, nil, nil, vm->globals_instances);
  if (!ctx_inst) {
    return NULL;
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack, ctx_inst)) {
    return NULL;
  }
  RunContext_ptr ctx =
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  ctx->ctx_code = code;
  Xen_Instance* retval = vm_run_ctx(ctx);
  if (!retval) {
    callable_free(code);
    return NULL;
  }
  callable_free(code);
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  return retval;
}
