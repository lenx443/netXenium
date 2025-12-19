#ifndef __XEN_BOOLEAN_H__
#define __XEN_BOOLEAN_H__

#include "instance.h"

#define Xen_True (Xen_ABI_Get_True())
#define Xen_False (Xen_ABI_Get_False())

Xen_bool_t Xen_IsBoolean(Xen_Instance*);
Xen_Instance* Xen_ABI_Get_True(void);
Xen_Instance* Xen_ABI_Get_False(void);

#endif
