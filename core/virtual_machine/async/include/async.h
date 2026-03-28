#ifndef __ASYNC_H__
#define __ASYNC_H__

#include "instance.h"
#include "xen_typedefs.h"

#define Xen_CORO_CREATED 1
#define Xen_CORO_TERMINATED 2
#define Xen_CORO_EXCEPTED 3
#define Xen_CORO_RESUME 4
#define Xen_CORO_PAUSE 5

Xen_Instance* Xen_Async_Run(Xen_Instance*);

Xen_bool_t Xen_Async_Get_Active(void);
Xen_Instance* Xen_Async_Get_EventLoop(void);
void Xen_Async_Set_Active(Xen_bool_t);

#endif
