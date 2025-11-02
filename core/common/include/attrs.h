#ifndef __ATTRS_H__
#define __ATTRS_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Attr_Get(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Attr_Get_Str(Xen_Instance*, const char*);
Xen_Instance* Xen_Attr_String(Xen_Instance*);
const char* Xen_Attr_String_Str(Xen_Instance*);
Xen_Instance* Xen_Attr_Boolean(Xen_Instance*);
Xen_Instance* Xen_Attr_Raw(Xen_Instance*);
const char* Xen_Attr_Raw_Str(Xen_Instance*);
Xen_Instance* Xen_Attr_Index_Get(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Attr_Index_Size_Get(Xen_Instance*, Xen_size_t);
Xen_Instance* Xen_Attr_Index_Set(Xen_Instance*, Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Attr_Index_Size_Set(Xen_Instance*, Xen_size_t, Xen_Instance*);

#endif
