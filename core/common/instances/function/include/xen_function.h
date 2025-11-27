#ifndef __XEN_FUNCTION_H__
#define __XEN_FUNCTION_H__

#include "callable.h"
#include "instance.h"

Xen_INSTANCE* Xen_Function_From_Native(Xen_Native_Func, Xen_Instance*);
Xen_INSTANCE* Xen_Function_From_Callable(CALLABLE_ptr, Xen_Instance*,
                                         Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Function_Call(Xen_Instance*, Xen_Instance*, Xen_Instance*);

#endif
