#ifndef __XEN_EVENTLOOP_H__
#define __XEN_EVENTLOOP_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_EventLoop_New(void);
void Xen_EventLoop_Task_Push(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_EventLoop_Task_Pop(Xen_Instance*);
int Xen_EventLoop_Task_Empty(Xen_Instance*);
void Xen_EventLoop_Timer_Push(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_EventLoop_Timer_Pop(Xen_Instance*);
Xen_Instance* Xen_EventLoop_Timer_Peek(Xen_Instance*);
int Xen_EventLoop_Timer_Empty(Xen_Instance*);
void Xen_EventLoop_CB_Interrupt_Call(Xen_Instance*);
void Xen_EventLoop_SCB_Interrupt(Xen_Instance*, Xen_Instance*);
void Xen_EventLoop_Set_Resumed(Xen_Instance*, Xen_Instance*);
Xen_Instance*Xen_EventLoop_Get_Resumed(Xen_Instance*);
Xen_size_t Xen_EventLoop_IO_Ref(Xen_Instance*);
void Xen_EventLoop_IO_Inc(Xen_Instance*);
void Xen_EventLoop_IO_Dec(Xen_Instance*);

#endif
