#ifndef __VM_H__
#define __VM_H__

#include <stdbool.h>

#include "implement.h"
#include "instance.h"
#include "xen_typedefs.h"

bool Xen_VM_Store_Native_Function(Xen_Instance*, const char*, Xen_Native_Func,
                                  Xen_Instance*);
bool Xen_VM_Store_Native_Function_Async(Xen_Instance*, const char*, Xen_Native_Func_Async, Xen_size_t);
int Xen_VM_Except_Throw(Xen_Instance*);

#endif
