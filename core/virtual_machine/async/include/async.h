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
void Xen_Async_Set_Active(Xen_bool_t);

Xen_Instance* Xen_Async_Get_Resumed(void);
void Xen_Async_Set_Resumed(Xen_Instance*);

void Xen_Async_Push(Xen_Instance*);
Xen_Instance* Xen_Async_Pop(void);

#endif
