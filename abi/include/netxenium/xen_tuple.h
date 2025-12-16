#ifndef __XEN_TUPLE_H__
#define __XEN_TUPLE_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Tuple_From_Array(Xen_size_t, Xen_Instance**);
Xen_Instance* Xen_Tuple_From_Vector(Xen_Instance*);
Xen_Instance* Xen_Tuple_Get_Index(Xen_Instance*, Xen_size_t);
Xen_Instance* Xen_Tuple_Peek_Index(Xen_Instance*, Xen_size_t);

#endif
