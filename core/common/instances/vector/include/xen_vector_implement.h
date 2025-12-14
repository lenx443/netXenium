#ifndef __XEN_VECTOR_IMPLEMENT_H__
#define __XEN_VECTOR_IMPLEMENT_H__

#include "implement.h"

struct __Implement* Xen_Vector_GetImplement(void);
int Xen_Vector_Init(void);
void Xen_Vector_Finish(void);

#endif
