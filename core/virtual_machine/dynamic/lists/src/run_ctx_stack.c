#include <stdlib.h>

#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "run_frame.h"
#include "vm_def.h"
#include "xen_nil.h"
#include "xen_vector.h"

int run_context_stack_push(RunContext_Stack_ptr *ctx_stack, Xen_Instance *closure,
                           Xen_Instance *caller, struct __Instance *self,
                           Xen_Instance *args) {
  if (!ctx_stack) { return 0; }
  RunContext_Stack_ptr ctx_stack_new = malloc(sizeof(RunContext_Stack));
  if (!ctx_stack_new) { return 0; }
  Xen_Instance *alloc_args =
      Xen_Vector_From_Array(4, (Xen_Instance *[]){caller, closure, self, args});
  if (!alloc_args) {
    free(ctx_stack_new);
    return 0;
  }
  ctx_stack_new->ctx = (RunContext_ptr)__instance_new(&Xen_Run_Frame, alloc_args, 0);
  if_nil_eval(ctx_stack_new->ctx) {
    Xen_DEL_REF(alloc_args);
    free(ctx_stack_new);
    return 0;
  }
  Xen_DEL_REF(alloc_args);
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

Xen_Instance *run_context_stack_peek_top(RunContext_Stack_ptr *ctx_stack) {
  if (!ctx_stack) { return nil; }
  return (Xen_Instance *)(*ctx_stack)->ctx;
}

void run_context_stack_pop_top(RunContext_Stack_ptr *ctx_stack) {
  if (!ctx_stack || !*ctx_stack) { return; }
  RunContext_Stack_ptr temp = *ctx_stack;
  *ctx_stack = (*ctx_stack)->next;
  Xen_DEL_REF(temp->ctx);
  free(temp);
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
