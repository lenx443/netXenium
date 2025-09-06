#ifndef __XEN_STRING_H__
#define __XEN_STRING_H__

#include <stdbool.h>

#include "instance.h"

Xen_INSTANCE *Xen_String_From_CString(const char *);
const char *Xen_String_As_CString(Xen_INSTANCE *);
const char Xen_String_As_Char(Xen_Instance *);

#endif
