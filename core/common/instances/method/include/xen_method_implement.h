#ifndef __XEN_METHOD_IMPLEMENT_H__
#define __XEN_METHOD_IMPLEMENT_H__

#include "implement.h"

struct __Implement* Xen_Method_GetImplement(void);

int Xen_Method_Init(void);
void Xen_Method_Finish(void);

#endif
