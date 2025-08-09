#include <stdlib.h>

#include "call_args.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"

int run_context_stack_push(RunContext_Stack_ptr *ctx_stack, RunContext_ptr caller,
                           struct __Instance *self, CallArgs *args) {
  if (!ctx_stack) { return 0; }
  RunContext_Stack_ptr ctx_stack_new = malloc(sizeof(RunContext_Stack));
  if (!ctx_stack_new) { return 0; }
  ctx_stack_new->ctx = run_context_new(caller, self, args);
  if (!ctx_stack_new->ctx) {
    free(ctx_stack_new);
    return 0;
  }
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
    run_context_free(current->ctx);
    free(current);
    current = next;
  }
  *ctx_stack = NULL;
}
