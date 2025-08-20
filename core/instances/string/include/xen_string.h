#ifndef __XEN_STRING_H__
#define __XEN_STRING_H__

#include <stdbool.h>

#include "xen_string_instance.h"

Xen_String *Xen_String_From_CString(const char *);
const char *Xen_String_As_CString(Xen_String *);

#endif
