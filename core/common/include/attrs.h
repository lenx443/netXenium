#ifndef __ATTRS_H__
#define __ATTRS_H__

#include "instance.h"

Xen_Instance* Xen_Attr_Get(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Attr_Get_Str(Xen_Instance*, const char*);
Xen_Instance* Xen_Attr_String(Xen_Instance*);
const char* Xen_Attr_String_Str(Xen_Instance*);
Xen_Instance* Xen_Attr_Raw(Xen_Instance*);
const char* Xen_Attr_Raw_Str(Xen_Instance*);

#endif
