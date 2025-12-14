#ifndef __XEN_EXCEPT_IMPLEMENT_H__
#define __XEN_EXCEPT_IMPLEMENT_H__

#include "implement.h"

struct __Implement* Xen_Except_GetImplement(void);
int Xen_Except_Init(void);
void Xen_Except_Finish(void);

#endif
