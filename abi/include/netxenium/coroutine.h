#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include "instance.h"

#define Xen_CORO_CREATED 1
#define Xen_CORO_TERMINATED 2
#define Xen_CORO_EXCEPTED 3
#define Xen_CORO_RESUME 4
#define Xen_CORO_PAUSE 5

#define Xen_COROUTINE_RETURN(r) \
  Xen_Coroutine_Return(coro, r); \
  return;

void* Xen_Coroutine_Data(Xen_Instance*);
void Xen_Coroutine_SStatus(Xen_Instance*, int);
int Xen_Coroutine_GStatus(Xen_Instance*);
void Xen_Coroutine_Return(Xen_Instance*, Xen_Instance*);

#endif
