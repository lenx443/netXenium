#ifndef __XEN_CSTRINGS_H__
#define __XEN_CSTRINGS_H__

#include "xen_typedefs.h"

char* Xen_CString_From_Pointer(void*);
char* Xen_CString_As_Raw(const char*);
Xen_size_t Xen_CString_Len(const char*);
char* Xen_CString_Dup(const char*);

#endif
