#ifndef __XEN_CSTRINGS_H__
#define __XEN_CSTRINGS_H__

#include "xen_typedefs.h"

Xen_string_t Xen_CString_From_Pointer(void*);
Xen_string_t Xen_CString_As_Raw(Xen_c_string_t);
Xen_size_t Xen_CString_Len(Xen_c_string_t);
Xen_string_t Xen_CString_Dup(Xen_c_string_t);

#endif
