#ifndef __XEN_CBUFFER_H__
#define __XEN_CBUFFER_H__

#include "instance.h"
#include "xen_typedefs.h"

typedef struct {
  Xen_string_t buf;
  Xen_size_t length;
  Xen_size_t cap;
} Xen_CBuffer;

Xen_CBuffer* Xen_CBuffer_New(void);
Xen_CBuffer* Xen_CBuffer_New_From_CStr(Xen_c_string_t);
void Xen_CBuffer_Append_Char(Xen_CBuffer*, char);
void Xen_CBuffer_Append_CStr(Xen_CBuffer*, Xen_c_string_t);
Xen_string_t Xen_CBuffer_As_CStr(Xen_CBuffer*);
Xen_Instance* Xen_CBuffer_As_String(Xen_CBuffer*);
void Xen_CBuffer_Free(Xen_CBuffer*);

#endif
