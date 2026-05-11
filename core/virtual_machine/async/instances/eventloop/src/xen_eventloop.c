#include "xen_eventloop.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_eventloop_instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_queue.h"
#include "xen_timer_heap.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>

Xen_Instance* Xen_EventLoop_New(void) {
  Xen_EventLoop* eloop = (Xen_EventLoop*)__instance_new(xen_globals->implements->eventloop, nil, nil, 0);
  Xen_GC_Write_Field(&eloop->tasks, (Xen_GCHeader*)Xen_Queue_New());
  eloop->event_fd = epoll_create1(0);
  eloop->timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = eloop->timer_fd;
  epoll_ctl(eloop->event_fd, EPOLL_CTL_ADD, eloop->timer_fd, &ev);
  return (Xen_Instance*)eloop;
}

void Xen_EventLoop_Task_Push(Xen_Instance *eloop, Xen_Instance *task) {
  Xen_Queue_Push((Xen_Instance*)((Xen_EventLoop*)eloop)->tasks->ptr, task);
}

Xen_Instance* Xen_EventLoop_Task_Pop(Xen_Instance *eloop) {
  return Xen_Queue_Pop((Xen_Instance*)((Xen_EventLoop*)eloop)->tasks->ptr);
}

int Xen_EventLoop_Task_Empty(Xen_Instance *eloop) {
  return Xen_Queue_Empty((Xen_Instance*)((Xen_EventLoop*)eloop)->tasks->ptr);
}

void Xen_EventLoop_Timer_Push(Xen_Instance* eloop, Xen_Instance* timer) {
  Xen_Timer_Heap_Push((Xen_Timer_Heap*)((Xen_EventLoop*)eloop)->timer_heap->ptr, timer);
}

Xen_Instance* Xen_EventLoop_Timer_Pop(Xen_Instance* eloop) {
  return Xen_Timer_Heap_Pop((Xen_Timer_Heap*)((Xen_EventLoop*)eloop)->timer_heap->ptr);
}

Xen_Instance* Xen_EventLoop_Timer_Peek(Xen_Instance* eloop) {
  return Xen_Timer_Heap_Peek((Xen_Timer_Heap*)((Xen_EventLoop*)eloop)->timer_heap->ptr);
}
int Xen_EventLoop_Timer_Empty(Xen_Instance* eloop) {
  return Xen_Timer_Heap_Empty((Xen_Timer_Heap*)((Xen_EventLoop*)eloop)->timer_heap->ptr);
}

void Xen_EventLoop_Set_Resumed(Xen_Instance *eloop, Xen_Instance *task) {
  Xen_GC_Write_Field(&((Xen_EventLoop*)eloop)->resumed, (Xen_GCHeader*)task);
}

Xen_Instance*Xen_EventLoop_Get_Resumed(Xen_Instance* eloop) {
  return (Xen_Instance*)((Xen_EventLoop*)eloop)->resumed->ptr;
}
