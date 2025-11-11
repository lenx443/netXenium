#ifndef __XEN_FUNCTION_H__
#define __XEN_FUNCTION_H__

#include "callable.h"
#include "instance.h"
#include "program_code.h"

Xen_INSTANCE* Xen_Function_From_Native(Xen_Native_Func, Xen_Instance*);
Xen_INSTANCE* Xen_Function_From_Program(ProgramCode_t, Xen_Instance*);
Xen_Instance* Xen_Function_Call(Xen_Instance*, Xen_Instance*, Xen_Instance*);

#endif
