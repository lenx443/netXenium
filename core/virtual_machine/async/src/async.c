#include "async.h"
#include "coroutine_instance.h"
#include "vm_backtrace.h"
#include "vm_run.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_queue.h"

Xen_Instance* Xen_Async_Run(Xen_Instance* new_coro) {
  Xen_Async_Set_Active(1);
  Xen_Async_Push(new_coro);
  Xen_Instance* coro_inst = NULL;
  while ((coro_inst = Xen_Async_Pop()) != NULL) {
    Xen_Coroutine* coro = (Xen_Coroutine*)coro_inst;
    switch (coro->status) {
    case Xen_CORO_CREATED:
      coro->status = Xen_CORO_RESUME;
      Xen_Async_Push(coro_inst);
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
      Xen_Async_Push(coro_inst);
      break;
    case Xen_CORO_RESUME:
      Xen_Async_Set_Resumed(coro_inst);
      vm_run((Xen_Instance*)coro->context->ptr);
      Xen_Async_Push(coro_inst);
      break;
    case Xen_CORO_PAUSE:
      if (((Xen_Coroutine*)coro->await->ptr)->status == Xen_CORO_TERMINATED) {
        coro->status = Xen_CORO_RESUME;
      }
      Xen_Async_Push(coro_inst);
      break;
    default:
      Xen_Async_Set_Active(0);
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
  return nil;
}

Xen_bool_t Xen_Async_Get_Active(void) {
  return ((*xen_globals->vm)->evloop.active);
}

void Xen_Async_Set_Active(Xen_bool_t val) {
  (*xen_globals->vm)->evloop.active = XEN_BOOL(val);
  if (!val) {
    (*xen_globals->vm)->evloop.resumed = NULL;
  }
}

Xen_Instance* Xen_Async_Get_Resumed(void) {
  return (Xen_Instance*)(*xen_globals->vm)->evloop.resumed->ptr;
}

void Xen_Async_Set_Resumed(Xen_Instance* val) {
  Xen_GC_Write_Field(&(*xen_globals->vm)->evloop.resumed, (struct __GC_Header *)val);
}

void Xen_Async_Push(Xen_Instance* value) {
  Xen_Queue_Push((Xen_Instance*)(*xen_globals->vm)->evloop.tasks->ptr, value);
}

Xen_Instance* Xen_Async_Pop(void) {
  return Xen_Queue_Pop((Xen_Instance*)(*xen_globals->vm)->evloop.tasks->ptr);
}
