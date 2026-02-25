#ifndef __XEN_QUEUE_IMPLEMENT_H__
#define __XEN_QUEUE_IMPLEMENT_H__

#include "implement.h"

struct __Implement* Xen_Queue_GetImplement(void);

int Xen_Queue_Init(void);
void Xen_Queue_Finish(void);

#endif
