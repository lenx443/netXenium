#include "async.h"
#include "coroutine_instance.h"
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
    case Xen_CORO_RESUME:
      Xen_Async_Set_Resumed(coro_inst);
      vm_run((Xen_Instance*)coro->context->ptr);
      Xen_Async_Push(coro_inst);
      break;
    case Xen_CORO_PAUSE:
      Xen_Async_Push(coro_inst);
      break;
    case Xen_CORO_TERMINATE:
      break;
    default:
      Xen_Async_Set_Active(0);
      return NULL;
    }
  }
  Xen_Async_Set_Active(0);
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
  Xen_GC_Write_Field((struct __GC_Header *)(*xen_globals->vm),
                     (struct __GC_Handle **)&(*xen_globals->vm)->evloop.resumed,
                     (struct __GC_Header *)val);
}

void Xen_Async_Push(Xen_Instance* value) {
  Xen_Queue_Push((Xen_Instance*)(*xen_globals->vm)->evloop.tasks->ptr, value);
}

Xen_Instance* Xen_Async_Pop(void) {
  return Xen_Queue_Pop((Xen_Instance*)(*xen_globals->vm)->evloop.tasks->ptr);
}
