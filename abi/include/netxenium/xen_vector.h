#ifndef __XEN_VECTOR_H__
#define __XEN_VECTOR_H__

#include <stddef.h>

#include "instance.h"

Xen_Instance* Xen_Vector_New(void);
Xen_Instance* Xen_Vector_From_Array(size_t, Xen_Instance**);
int Xen_Vector_Push(Xen_Instance*, Xen_Instance*);
int Xen_Vector_Push_Vector(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Vector_Pop(Xen_Instance*);
Xen_Instance* Xen_Vector_Top(Xen_Instance*);
Xen_Instance* Xen_Vector_Get_Index(Xen_Instance*, size_t);
Xen_Instance* Xen_Vector_Peek_Index(Xen_Instance*, size_t);
size_t Xen_Vector_Size(Xen_Instance*);

#endif
