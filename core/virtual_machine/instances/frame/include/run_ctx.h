#ifndef __RUN_CTX_H__
#define __RUN_CTX_H__

#include "callable.h"
#include "instance.h"
#include "vm_scope.h"

#define CTX_GET_FLAG(ctx, flag)                                                \
  (((((struct RunContext*)ctx)->ctx_flags) & (flag)) != 0)

Xen_Instance* Xen_Ctx_New(Xen_Instance*, Xen_Instance*, Xen_Instance*, Xen_Instance*,
                          Xen_Instance*, Xen_Instance*, Xen_Instance*, Xen_VM_Scopes*, CALLABLE_ptr);

#endif
