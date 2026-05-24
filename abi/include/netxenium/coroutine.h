#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include "instance.h"

#define Xen_CORO_CREATED  1
#define Xen_CORO_RUNNING  2
#define Xen_CORO_FINISHED 3

void* Xen_Coroutine_Data(Xen_Instance*);
void Xen_Coroutine_SStatus(Xen_Instance*, int);
int Xen_Coroutine_GStatus(Xen_Instance*);

#endif
