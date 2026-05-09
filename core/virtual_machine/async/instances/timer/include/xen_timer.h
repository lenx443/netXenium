#ifndef __XEN_TIMER_H__
#define __XEN_TIMER_H__

#include "instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Timer_New(Xen_Instance*, Xen_uint64_t);
Xen_Instance* Xen_Timer_Coroutine(Xen_Instance*);
Xen_uint64_t Xen_Timer_Expire(Xen_Instance*);
void Xen_Timer_SIndex(Xen_Instance*, Xen_size_t);
Xen_size_t Xen_Timer_GIndex(Xen_Instance*);
int Xen_Timer_GCancelled(Xen_Instance*);
void Xen_Timer_SCancelled_True(Xen_Instance*);
void Xen_Timer_SCancelled_False(Xen_Instance*);
Xen_uint64_t Xen_Timer_Now_MS(void);

#endif
