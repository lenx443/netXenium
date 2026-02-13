#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include "instance.h"

#define Xen_CORO_CREATED  1
#define Xen_CORO_RUNNING  2
#define Xen_CORO_FINISHED 3

Xen_Instance* Xen_Coroutine_New(Xen_Instance*);

#endif
