#ifndef __XEN_ALLOC_H__
#define __XEN_ALLOC_H__

#include "xen_typedefs.h"

void* Xen_Alloc(Xen_size_t);
void* Xen_ZAlloc(Xen_size_t, Xen_size_t);
void* Xen_Realloc(void*, Xen_size_t);
void Xen_Dealloc(void*);

#endif
