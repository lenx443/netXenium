#ifndef __RUN_CTX_STACK_H__
#define __RUN_CTX_STACK_H__

#include "run_ctx.h"

typedef struct RunContext_Stack {
  RunContext_ptr ctx;
  struct RunContext_Stack *next;
} RunContext_Stack;

typedef RunContext_Stack *RunContext_Stack_ptr;

RunContext_Stack_ptr run_context_stack_new();
void run_context_stack_free(RunContext_Stack_ptr);

#endif
