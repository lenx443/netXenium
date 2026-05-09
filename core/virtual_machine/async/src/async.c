#include "async.h"
#include "coroutine_instance.h"
#include "gc_header.h"
#include "vm_backtrace.h"
#include "vm_run.h"
#include "xen_eventloop.h"
#include "xen_eventloop_instance.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_timer_heap.h"
#include "xen_typedefs.h"
#include "xen_timer.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#ifndef EPOLL_MAX_EVENTS
#define EPOLL_MAX_EVENTS 16
#endif

static void program_timerfd(int tfd, Xen_uint64_t next_expiration) {
    struct itimerspec ts = {0};
    Xen_uint64_t now = Xen_Timer_Now_MS();
    Xen_uint64_t delta = (next_expiration > now) ? (next_expiration - now) : 0;
    ts.it_value.tv_sec  = delta / 1000;
    ts.it_value.tv_nsec = (delta % 1000) * 1000000;
    timerfd_settime(tfd, 0, &ts, NULL);
}

void Xen_Async_Run(Xen_Instance* new_coro) {
  int __last_active = (*xen_globals->vm)->evloop.active;
  Xen_Instance* __last_evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  Xen_IGC_Push(__last_evloop);
  Xen_Instance* eloop = Xen_EventLoop_New();
  (*xen_globals->vm)->evloop.active = 1;
  Xen_IGC_WRITE_FIELD((*xen_globals->vm)->evloop.evloop, eloop);
  Xen_EventLoop_Task_Push(eloop, new_coro);
  struct epoll_event events[EPOLL_MAX_EVENTS];
  while (!Xen_EventLoop_Task_Empty(eloop)) {
    Xen_Async_Run_Tasks();
    int n = epoll_wait(((Xen_EventLoop*)eloop)->event_fd, events, EPOLL_MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
      if (events[i].data.fd == ((Xen_EventLoop*)eloop)->timer_fd) {
        Xen_uint64_t expirations;
        read(((Xen_EventLoop*)eloop)->timer_fd, &expirations, sizeof(expirations));
        Xen_Async_Run_Timers();
      }
    }
  }
  Xen_Async_Set_Active(0);
  Xen_Coroutine* coro = (Xen_Coroutine*)new_coro;
  if (coro->except.active) {
    (*xen_globals->vm)->except.active = 1;
    Xen_GC_Write_Field(&(*xen_globals->vm)->except.except, coro->except.except->ptr);
    vm_backtrace_copy(coro->except.bt, (*xen_globals->vm)->except.bt);
    vm_backtrace_clear(coro->except.bt);
    coro->except.active = 0;
  }
  Xen_IGC_Pop();
  Xen_IGC_WRITE_FIELD((*xen_globals->vm)->evloop.evloop, __last_evloop);
  (*xen_globals->vm)->evloop.active = __last_active;
}

void Xen_Async_Run_Tasks(void) {
  Xen_Instance* eloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  Xen_Instance* coro_inst = NULL;
  while ((coro_inst = Xen_EventLoop_Task_Pop(eloop)) != NULL) {
    Xen_Coroutine* coro = (Xen_Coroutine*)coro_inst;
    switch (coro->status) {
    case Xen_CORO_CREATED:
      coro->status = Xen_CORO_RESUME;
      Xen_EventLoop_Task_Push(eloop, coro_inst);
      break;
    case Xen_CORO_TERMINATED:
      if (coro->awaiter->ptr) {
        Xen_EventLoop_Task_Push(eloop, (Xen_Instance *)coro->awaiter->ptr);
      }
      break;
    case Xen_CORO_EXCEPTED:
      coro->except.active = 1;
      Xen_GC_Write_Field(&coro->except.except, (*xen_globals->vm)->except.except->ptr);
      vm_backtrace_copy((*xen_globals->vm)->except.bt, coro->except.bt);
      vm_backtrace_clear((*xen_globals->vm)->except.bt);
      (*xen_globals->vm)->except.active = 0;
      coro->status = Xen_CORO_TERMINATED;
      Xen_EventLoop_Task_Push(eloop, coro_inst);
      break;
    case Xen_CORO_RESUME:
      Xen_EventLoop_Set_Resumed(eloop, coro_inst);
      vm_run((Xen_Instance*)coro->context->ptr);
      Xen_EventLoop_Task_Push(eloop, coro_inst);
      break;
    case Xen_CORO_PAUSE:
      if (((Xen_Coroutine*)coro->awaited->ptr)->status == Xen_CORO_TERMINATED) {
        coro->status = Xen_CORO_RESUME;
        Xen_EventLoop_Task_Push(eloop, coro_inst);
      }
      break;
    default:
      return;
    }
  }
}

void Xen_Async_Run_Timers(void) {
  Xen_Instance* eloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  Xen_Timer_Heap* timer_heap = (Xen_Timer_Heap*)((Xen_EventLoop*)eloop)->timer_heap->ptr;
  Xen_uint64_t now = Xen_Timer_Now_MS();
  while (!Xen_Timer_Heap_Empty(timer_heap)) {
    Xen_Instance* timer = Xen_Timer_Heap_Peek(timer_heap);
    if (Xen_Timer_Expire(timer) > now) break;
    Xen_Timer_Heap_Pop(timer_heap);
    Xen_EventLoop_Task_Push(eloop, timer);
  }
  if (!Xen_Timer_Heap_Empty(timer_heap)) {
    Xen_Instance* next = Xen_Timer_Heap_Peek(timer_heap);
    program_timerfd(((Xen_EventLoop*)eloop)->timer_fd, Xen_Timer_Expire(next));
  }
}

Xen_bool_t Xen_Async_Get_Active(void) {
  return ((*xen_globals->vm)->evloop.active);
}

Xen_Instance* Xen_Async_Get_EventLoop(void) {
  return (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
}

void Xen_Async_Set_Active(Xen_bool_t val) {
  (*xen_globals->vm)->evloop.active = XEN_BOOL(val);
}
