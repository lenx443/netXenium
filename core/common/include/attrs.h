#ifndef __ATTRS_H__
#define __ATTRS_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Attr_Get(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Attr_Get_Str(Xen_Instance*, const char*);
int Xen_Attr_Set(Xen_Instance*, Xen_Instance*, Xen_Instance*);
int Xen_Attr_Set_Str(Xen_Instance*, const char*, Xen_Instance*);
Xen_Instance* Xen_Attr_String(Xen_Instance*);
const char* Xen_Attr_String_Str(Xen_Instance*);
Xen_Instance* Xen_Attr_String_Stack(Xen_Instance*, Xen_Instance*);
const char* Xen_Attr_String_Stack_Str(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Attr_Raw(Xen_Instance*);
const char* Xen_Attr_Raw_Str(Xen_Instance*);
Xen_Instance* Xen_Attr_Raw_Stack(Xen_Instance*, Xen_Instance*);
const char* Xen_Attr_Raw_Stack_Str(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Attr_Boolean(Xen_Instance*);
Xen_Instance* Xen_Attr_Index_Get(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Attr_Index_Size_Get(Xen_Instance*, Xen_size_t);
int Xen_Attr_Index_Set(Xen_Instance*, Xen_Instance*, Xen_Instance*);
int Xen_Attr_Index_Size_Set(Xen_Instance*, Xen_size_t, Xen_Instance*);
Xen_Instance* Xen_Attr_Iter(Xen_Instance*);
Xen_Instance* Xen_Attr_Next(Xen_Instance*);

#endif
