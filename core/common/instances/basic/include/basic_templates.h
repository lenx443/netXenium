#ifndef __BASIC_TEMPLATES_H__
#define __BASIC_TEMPLATES_H__

#include "implement.h"
#include "instance.h"

Xen_Instance* Xen_Basic_New(Xen_c_string_t, Xen_Instance*, Xen_Implement*);
void Xen_Basic_Mapped_Trace(Xen_Instance*);
Xen_Instance* Xen_Basic_String(Xen_Instance*, Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Basic_Get_Attr_Static(Xen_Instance*, Xen_Instance*,
                                        Xen_Instance*);
Xen_Instance* Xen_Basic_Set_Attr_Static(Xen_Instance*, Xen_Instance*,
                                        Xen_Instance*);
Xen_Instance* Xen_Basic_Get_Attr_Mapped(Xen_Instance*, Xen_Instance*,
                                        Xen_Instance*);
Xen_Instance* Xen_Basic_Set_Attr_Mapped(Xen_Instance*, Xen_Instance*,
                                        Xen_Instance*);

#endif
