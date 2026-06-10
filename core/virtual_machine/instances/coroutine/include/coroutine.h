#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include "instance.h"
#include "xen_igc.h"

#define Xen_CORO_CREATED 1
#define Xen_CORO_TERMINATED 2
#define Xen_CORO_RESUME 3
#define Xen_CORO_PAUSE 4

#define Xen_COROUTINE_RETURN(r)  \
  Xen_Coroutine_Return(coro, r); \
  return;

#define Xen_COROUTINE_EXCEPTED   \
  Xen_Coroutine_Excepted(coro); \
  return;

Xen_Instance* Xen_Coroutine_New(Xen_Instance*);
Xen_Instance* Xen_Coroutine_New_Native(Xen_Native_Func_Async, Xen_Instance*, Xen_Instance*, Xen_Instance*, Xen_size_t);
void* Xen_Coroutine_Data(Xen_Instance*);
Xen_IGC_Fork* Xen_Coroutine_IGC_Fork(Xen_Instance*);
void Xen_Coroutine_SStatus(Xen_Instance*, int);
int Xen_Coroutine_GStatus(Xen_Instance*);
int Xen_Coroutine_Await(Xen_Instance*, Xen_Instance*);
Xen_Instance* Xen_Coroutine_Await_Resume(Xen_Instance*);
void Xen_Coroutine_Return(Xen_Instance*, Xen_Instance*);
void Xen_Coroutine_Excepted(Xen_Instance*);

#endif
