#include "run_ctx_stack.h"
#include "gc_header.h"
#include "instance.h"
#include "run_ctx_instance.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_life.h"

int run_context_stack_push(RunContext_Stack_ptr* ctx_stack, Xen_Instance* ctx) {
  if (!ctx_stack) {
    return 0;
  }
  RunContext_Stack_ptr ctx_stack_new = Xen_Alloc(sizeof(RunContext_Stack));
  if (!ctx_stack_new) {
    return 0;
  }
  ctx_stack_new->ctx = (RunContext_ptr)ctx;
  Xen_GC_Push_Root((Xen_GCHeader*)ctx_stack_new->ctx);
  ctx_stack_new->ctx->ctx_running = 1;
  ctx_stack_new->ctx->ctx_id = ++(*xen_globals->vm)->ctx_id_count;
  ctx_stack_new->next = NULL;
  if (*ctx_stack) {
    ctx_stack_new->next = *ctx_stack;
    *ctx_stack = ctx_stack_new;
    return 1;
  }
  *ctx_stack = ctx_stack_new;
  return 1;
}

Xen_Instance* run_context_stack_peek_top(RunContext_Stack_ptr* ctx_stack) {
  if (!ctx_stack || !*ctx_stack) {
    return NULL;
  }
  return (Xen_Instance*)(*ctx_stack)->ctx;
}

void run_context_stack_pop_top(RunContext_Stack_ptr* ctx_stack) {
  if (!ctx_stack || !*ctx_stack) {
    return;
  }
  RunContext_Stack_ptr temp = *ctx_stack;
  *ctx_stack = (*ctx_stack)->next;
  --(*xen_globals->vm)->ctx_id_count;
  Xen_GC_Pop_Root();
  Xen_Dealloc(temp);
}

void run_context_stack_free(RunContext_Stack_ptr* ctx_stack) {
  if (!ctx_stack || !*ctx_stack) {
    return;
  }
  RunContext_Stack_ptr current = *ctx_stack;
  while (current) {
    RunContext_Stack_ptr next = current->next;
    Xen_GC_Pop_Root();
    Xen_Dealloc(current);
    current = next;
  }
  (*xen_globals->vm)->ctx_id_count = 0;
  *ctx_stack = NULL;
}
