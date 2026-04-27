#ifndef __XEN_EVENTLOOP_H__
#define __XEN_EVENTLOOP_H__

#include "instance.h"

Xen_Instance* Xen_EventLoop_New(void);
void Xen_EventLoop_Task_Push(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_EventLoop_Task_Pop(Xen_Instance*);
int Xen_EventLoop_Task_Empty(Xen_Instance*);
void Xen_EventLoop_Set_Resumed(Xen_Instance*, Xen_Instance*);
Xen_Instance*Xen_EventLoop_Get_Resumed(Xen_Instance*);

#endif
