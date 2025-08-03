#ifndef __RUN_CTX_STACK_H__
#define __RUN_CTX_STACK_H__

#include "call_args.h"
#include "run_ctx.h"

typedef struct RunContext_Stack {
  RunContext_ptr ctx;
  struct RunContext_Stack *next;
} RunContext_Stack;

typedef RunContext_Stack *RunContext_Stack_ptr;

int run_context_stack_push(RunContext_Stack_ptr *, CallArgs *);
RunContext_ptr run_context_stack_peek_top(RunContext_Stack_ptr *);
RunContext_ptr run_context_stack_pop_top(RunContext_Stack_ptr *);
void run_context_stack_free(RunContext_Stack_ptr *);

#endif
