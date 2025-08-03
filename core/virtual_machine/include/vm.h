#ifndef __VM_H__
#define __VM_H__

#include "call_args.h"
#include "callable.h"
#include "run_ctx.h"

void vm_ctx_clear(RunContext_ptr);
int vm_new_ctx_callable(CALLABLE_ptr, CallArgs *);
int vm_run_callable(CALLABLE_ptr, CallArgs *);
void vm_run_ctx(RunContext_ptr);

#endif
