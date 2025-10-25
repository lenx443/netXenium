#ifndef __XEN_STRING_H__
#define __XEN_STRING_H__

#include <stdbool.h>

#include "instance.h"

Xen_INSTANCE* Xen_String_From_CString(const char*);
Xen_Instance* Xen_String_From_Char(char);
Xen_Instance* Xen_String_From_Concat(Xen_Instance*, Xen_Instance*);
const char* Xen_String_As_CString(Xen_INSTANCE*);
char Xen_String_As_Char(Xen_Instance*);
unsigned long Xen_String_Hash(Xen_Instance*);

#endif
