#include <stdlib.h>

#include "call_args.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "run_frame.h"
#include "vm_def.h"

int run_context_stack_push(RunContext_Stack_ptr *ctx_stack, RunContext_ptr caller,
                           struct __Instance *self, CallArgs *args) {
  if (!ctx_stack) { return 0; }
  RunContext_Stack_ptr ctx_stack_new = malloc(sizeof(RunContext_Stack));
  if (!ctx_stack_new) { return 0; }
  struct CArg arg3 = {CARG_POINTER, args, sizeof(*args)};
  CallArgs *alloc_args =
      call_args_create(3, (struct CArg[]){
                              {CARG_INSTANCE, caller, sizeof(struct RunContext)},
                              {CARG_INSTANCE, self, sizeof(*self)},
                              {CARG_POINTER, args, sizeof(*args)},
                          });
  if (!alloc_args) {
    free(ctx_stack_new);
    return 0;
  }
  ctx_stack_new->ctx = (RunContext_ptr)__instance_new(&Xen_Run_Frame, alloc_args);
  if (!ctx_stack_new->ctx) {
    call_args_free(alloc_args);
    free(ctx_stack_new);
    return 0;
  }
  ctx_stack_new->ctx->ctx_id = ++vm->ctx_id_count;
  ctx_stack_new->next = NULL;
  if (*ctx_stack) {
    ctx_stack_new->next = *ctx_stack;
    *ctx_stack = ctx_stack_new;
    return 1;
  }
  *ctx_stack = ctx_stack_new;
  return 1;
}

int run_context_stack_push_refer(RunContext_Stack_ptr *ctx_stack, RunContext_ptr refer) {
  if (!ctx_stack) { return 0; }
  RunContext_Stack_ptr ctx_stack_new = malloc(sizeof(RunContext_Stack));
  if (!ctx_stack_new) { return 0; }
  ctx_stack_new->ctx = refer;
  Xen_ADD_REF(refer);
  ctx_stack_new->ctx->ctx_id = ++vm->ctx_id_count;
  ctx_stack_new->next = NULL;
  if (*ctx_stack) {
    ctx_stack_new->next = *ctx_stack;
    *ctx_stack = ctx_stack_new;
    return 1;
  }
  *ctx_stack = ctx_stack_new;
  return 1;
}

RunContext_ptr run_context_stack_peek_top(RunContext_Stack_ptr *ctx_stack) {
  if (!ctx_stack) { return NULL; }
  return (*ctx_stack)->ctx;
}

RunContext_ptr run_context_stack_pop_top(RunContext_Stack_ptr *ctx_stack) {
  if (!ctx_stack || !*ctx_stack) { return NULL; }
  RunContext_Stack_ptr temp = *ctx_stack;
  *ctx_stack = (*ctx_stack)->next;
  RunContext_ptr result = temp->ctx;
  free(temp);
  return result;
}

void run_context_stack_free(RunContext_Stack_ptr *ctx_stack) {
  if (!ctx_stack || !*ctx_stack) { return; }
  RunContext_Stack_ptr current = *ctx_stack;
  while (current) {
    RunContext_Stack_ptr next = current->next;
    Xen_DEL_REF(current->ctx);
    free(current);
    current = next;
  }
  *ctx_stack = NULL;
}
