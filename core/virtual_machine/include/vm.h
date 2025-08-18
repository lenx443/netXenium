#ifndef __VM_H__
#define __VM_H__

#include <stdbool.h>

#include "call_args.h"
#include "callable.h"
#include "instance.h"
#include "instances_map.h"
#include "run_ctx.h"

#define VM_CHECK_ID(id) ((vm_current_ctx()->ctx_id) == (id))

bool vm_define_native_command(struct __Instances_Map *, Xen_INSTANCE *, const char *,
                              Xen_Native_Func);
bool vm_call_native_function(Xen_Native_Func, Xen_INSTANCE *, CallArgs *);
bool vm_call_basic_native_function(Xen_Native_Func, Xen_INSTANCE *);
Xen_INSTANCE *vm_get_prop_register(const char *, ctx_id_t);
RunContext_ptr vm_current_ctx();
void vm_ctx_clear(RunContext_ptr);
int vm_new_ctx_callable(CALLABLE_ptr, struct __Instance *, CallArgs *);
int vm_run_callable(CALLABLE_ptr, struct __Instance *, CallArgs *);
void vm_run_ctx(RunContext_ptr);

#endif
