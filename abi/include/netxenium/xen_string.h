#ifndef __XEN_STRING_H__
#define __XEN_STRING_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_bool_t Xen_IsString(Xen_Instance*);
Xen_Instance* Xen_String_From_CString(const char*);
Xen_Instance* Xen_String_From_Char(char);
Xen_Instance* Xen_String_From_Concat(Xen_Instance*, Xen_Instance*);
const char* Xen_String_As_CString(Xen_Instance*);
char Xen_String_As_Char(Xen_Instance*);
unsigned long Xen_String_Hash(Xen_Instance*);

#endif
