#ifndef __ASYNC_H__
#define __ASYNC_H__

#include "instance.h"
#include "xen_typedefs.h"

void Xen_Async_Run(Xen_Instance*);
void Xen_Async_Run_Tasks(void);
void Xen_Async_Run_Timers(void);

void Xen_Async_Scheduler_Timer(Xen_Instance*, Xen_Instance*);
Xen_bool_t Xen_Async_Get_Active(void);
Xen_Instance* Xen_Async_Get_EventLoop(void);
void Xen_Async_Set_Active(Xen_bool_t);

#endif
