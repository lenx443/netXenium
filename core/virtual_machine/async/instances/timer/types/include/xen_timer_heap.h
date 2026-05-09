#ifndef __XEN_TIMER_HEAP_H__
#define __XEN_TIMER_HEAP_H__

#include "gc_header.h"
#include "instance.h"
#include "xen_typedefs.h"

typedef struct {
  Xen_GCHeader gc;
  Xen_GCHandle** timers;
  Xen_size_t size;
  Xen_size_t capacity;
} Xen_Timer_Heap;

Xen_Timer_Heap* Xen_Timer_Heap_New(void);
int Xen_Timer_Heap_Empty(Xen_Timer_Heap*);
void Xen_Timer_Heap_Push(Xen_Timer_Heap*, Xen_Instance*);
Xen_Instance* Xen_Timer_Heap_Peek(Xen_Timer_Heap*);
Xen_Instance* Xen_Timer_Heap_Pop(Xen_Timer_Heap*);
void Xen_Timer_Heap_Remove(Xen_Timer_Heap*, Xen_Instance*);

#endif
