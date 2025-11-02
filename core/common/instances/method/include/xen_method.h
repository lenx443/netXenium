#ifndef __XEN_METHOD_H__
#define __XEN_METHOD_H__

#include "instance.h"

Xen_Instance* Xen_Method_New(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Method_Call(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Method_Attr_Call(Xen_Instance*, Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Method_Attr_Str_Call(Xen_Instance*, const char*,
                                       Xen_Instance*);

#endif
