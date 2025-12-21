#ifndef __XEN_BYTES_H__
#define __XEN_BYTES_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Bytes_New(void);
Xen_Instance* Xen_Bytes_From_Array(Xen_size_t, Xen_uint8_t*);
void Xen_Bytes_Append(Xen_Instance*, Xen_uint8_t);
void Xen_Bytes_Append_Array(Xen_Instance*, Xen_size_t, Xen_uint8_t*);

#endif
