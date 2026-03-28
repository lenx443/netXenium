#include "async.h"
#include "coroutine_instance.h"
#include "gc_header.h"
#include "vm_backtrace.h"
#include "vm_run.h"
#include "xen_eventloop.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_nil.h"

Xen_Instance* Xen_Async_Run(Xen_Instance* new_coro) {
  int __last_active = (*xen_globals->vm)->evloop.active;
  Xen_Instance* __last_evloop = (Xen_Instance*)(*xen_globals->vm)->evloop.evloop->ptr;
  Xen_IGC_Push(__last_evloop);
  Xen_Instance* eloop = Xen_EventLoop_New();
  (*xen_globals->vm)->evloop.active = 1;
  Xen_IGC_WRITE_FIELD((*xen_globals->vm)->evloop.evloop, eloop);
  Xen_EventLoop_Task_Push(eloop, new_coro);
  Xen_Instance* coro_inst = NULL;
  while ((coro_inst = Xen_EventLoop_Task_Pop(eloop)) != NULL) {
    Xen_Coroutine* coro = (Xen_Coroutine*)coro_inst;
    switch (coro->status) {
    case Xen_CORO_CREATED:
      coro->status = Xen_CORO_RESUME;
      Xen_EventLoop_Task_Push(eloop, coro_inst);
      break;
    case Xen_CORO_TERMINATED:
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
      if (((Xen_Coroutine*)coro->await->ptr)->status == Xen_CORO_TERMINATED) {
        coro->status = Xen_CORO_RESUME;
      }
      Xen_EventLoop_Task_Push(eloop, coro_inst);
      break;
    default:
      Xen_IGC_Pop();
      Xen_IGC_WRITE_FIELD((*xen_globals->vm)->evloop.evloop, __last_evloop);
      (*xen_globals->vm)->evloop.active = __last_active;
      return NULL;
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
  return nil;
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
