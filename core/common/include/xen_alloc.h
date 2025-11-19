#ifndef __XEN_ALLOC_H__
#define __XEN_ALLOC_H__

#include "implement.h"
#include "instance.h"
#include "xen_typedefs.h"

void* Xen_Alloc(Xen_size_t);
void* Xen_ZAlloc(Xen_size_t, Xen_size_t);
void* Xen_Realloc(void*, Xen_size_t);
void Xen_Dealloc(void*);

Xen_Instance* Xen_Instance_Alloc(Xen_Implement*);

#endif
