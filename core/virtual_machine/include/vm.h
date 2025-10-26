#ifndef __VM_H__
#define __VM_H__

#include <stdbool.h>

#include "callable.h"
#include "instance.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"

#define VM_CHECK_ID(id) ((run_ctx_id(vm_current_ctx())) == (id))

Xen_Instance* vm_current_ctx();
Xen_Instance* vm_root_ctx();
bool vm_define_native_function(Xen_Instance*, const char*, Xen_Native_Func,
                               Xen_Instance*);
Xen_Instance* vm_call_native_function(Xen_Native_Func, Xen_INSTANCE*,
                                      Xen_Instance*);
Xen_INSTANCE* vm_get_instance(const char*, ctx_id_t);
void vm_ctx_clear(RunContext_ptr);
int vm_new_ctx_callable(CALLABLE_ptr, Xen_Instance*, struct __Instance*,
                        Xen_Instance*);
Xen_Instance* vm_run_callable(CALLABLE_ptr, struct __Instance*, Xen_Instance*,
                              Xen_Instance*);

#endif
