#ifndef __BASIC_TEMPLATES_H__
#define __BASIC_TEMPLATES_H__

#include "instance.h"

Xen_Instance* Xen_Basic_String(Xen_Instance*, Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Basic_Get_Attr_Static(Xen_Instance*, Xen_Instance*,
                                        Xen_Instance*);
Xen_Instance* Xen_Basic_Set_Attr_Static(Xen_Instance*, Xen_Instance*,
                                        Xen_Instance*);

#endif
