#include "xen_eventloop.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_eventloop_instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_queue.h"

Xen_Instance* Xen_EventLoop_New(void) {
  Xen_EventLoop* eloop = (Xen_EventLoop*)__instance_new(xen_globals->implements->eventloop, nil, nil, 0);
  Xen_GC_Write_Field(&eloop->tasks, (Xen_GCHeader*)Xen_Queue_New());
  return (Xen_Instance*)eloop;
}

void Xen_EventLoop_Task_Push(Xen_Instance *eloop, Xen_Instance *task) {
  Xen_Queue_Push((Xen_Instance*)((Xen_EventLoop*)eloop)->tasks->ptr, task);
}

Xen_Instance* Xen_EventLoop_Task_Pop(Xen_Instance *eloop) {
  return Xen_Queue_Pop((Xen_Instance*)((Xen_EventLoop*)eloop)->tasks->ptr);
}

void Xen_EventLoop_Set_Resumed(Xen_Instance *eloop, Xen_Instance *task) {
  Xen_GC_Write_Field(&((Xen_EventLoop*)eloop)->resumed, (Xen_GCHeader*)task);
}

Xen_Instance*Xen_EventLoop_Get_Resumed(Xen_Instance* eloop) {
  return (Xen_Instance*)((Xen_EventLoop*)eloop)->resumed->ptr;
}
