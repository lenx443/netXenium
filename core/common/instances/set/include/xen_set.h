#ifndef __XEN_SET_H__
#define __XEN_SET_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Set_New();
Xen_Instance* Xen_Set_From_Array(Xen_size_t, Xen_Instance**);
Xen_Instance* Xen_Set_From_Array_Str(Xen_size_t, const char**);
int Xen_Set_Push(Xen_Instance*, Xen_Instance*);
int Xen_Set_Push_Str(Xen_Instance*, const char*);
int Xen_Set_Has(Xen_Instance*, Xen_Instance*);
int Xen_Set_Has_Str(Xen_Instance*, const char*);

#endif
