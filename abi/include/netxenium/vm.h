#ifndef __VM_H__
#define __VM_H__

#include <stdbool.h>

#include "implement.h"
#include "instance.h"

Xen_Instance* Xen_VM_Current_Ctx(void);
bool Xen_VM_Store_Global(const char*, Xen_Instance*);
bool Xen_VM_Store_Native_Function(Xen_Instance*, const char*, Xen_Native_Func,
                                  Xen_Instance*);
int Xen_VM_Except_Throw(Xen_Instance*);

#endif
