#ifndef __XEN_VECTOR_ITERATOR_IMPLEMENT_H__
#define __XEN_VECTOR_ITERATOR_IMPLEMENT_H__

#include "implement.h"

struct __Implement* Xen_Vector_Iterator_GetImplement(void);

int Xen_Vector_Iterator_Init(void);
void Xen_Vector_Iterator_Finish(void);

#endif
