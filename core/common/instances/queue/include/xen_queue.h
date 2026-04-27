#ifndef __XEN_QUEUE_H__
#define __XEN_QUEUE_H__

#include "instance.h"

Xen_Instance* Xen_Queue_New(void);
void Xen_Queue_Grow(Xen_Instance*);
void Xen_Queue_Push(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Queue_Pop(Xen_Instance*);
int Xen_Queue_Empty(Xen_Instance*);

#endif
