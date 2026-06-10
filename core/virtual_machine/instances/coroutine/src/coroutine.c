#include "coroutine.h"
#include "async.h"
#include "attrs.h"
#include "coroutine_instance.h"
#include "gc_header.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_alloc.h"
#include "xen_eventloop.h"
#include "xen_except.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_vector.h"
#include <string.h>

Xen_Instance* Xen_Coroutine_New(Xen_Instance* context) {
  Xen_Coroutine* coro = (Xen_Coroutine*)__instance_new(xen_globals->implements->coroutine, nil, nil, 0);
  coro->type = 1;
  Xen_GC_Write_Field(&coro->context, (struct __GC_Header *)context);
  Xen_Ctx_Enable_End(context);
  coro->status = Xen_CORO_CREATED;
  Xen_GC_Write_Field(&coro->result, (Xen_GCHeader*)nil);
  return (Xen_Instance*)coro;
}

Xen_Instance* Xen_Coroutine_New_Native(Xen_Native_Func_Async func, Xen_Instance* self,
                                       Xen_Instance* args, Xen_Instance*kwargs, Xen_size_t data_size) {
  Xen_Coroutine* coro = (Xen_Coroutine*)__instance_new(xen_globals->implements->coroutine, nil, nil, 0);
  coro->type = 2;
  coro->func_async = func;
  Xen_GC_Write_Field(&coro->self, (Xen_GCHeader*)self);
  Xen_GC_Write_Field(&coro->args, (Xen_GCHeader*)args);
  Xen_GC_Write_Field(&coro->kwargs, (Xen_GCHeader*)kwargs);
  if (data_size > 0) {
    coro->data = Xen_Alloc(data_size);
    memset(coro->data, 0, data_size);
  }
  coro->status = Xen_CORO_CREATED;
  Xen_GC_Write_Field(&coro->result, (Xen_GCHeader*)nil);
  return (Xen_Instance*)coro;
}

void* Xen_Coroutine_Data(Xen_Instance* coro) {
  return ((Xen_Coroutine*)coro)->data;
}

Xen_IGC_Fork* Xen_Coroutine_IGC_Fork(Xen_Instance* coro) {
  return (Xen_IGC_Fork*)((Xen_Coroutine*)coro)->igcfork->ptr;
}

void Xen_Coroutine_SStatus(Xen_Instance* coro, int status) {
  ((Xen_Coroutine*)coro)->status = status;
}

int Xen_Coroutine_GStatus(Xen_Instance* coro) {
  return ((Xen_Coroutine*)coro)->status;
}

int Xen_Coroutine_Await(Xen_Instance* cur, Xen_Instance* coro_inst) {
  if (!Xen_Async_Get_Active()) {
    Xen_AsyncError();
    return 0;
  }
  Xen_Coroutine* cur_coro = (Xen_Coroutine*)cur;
  if (Xen_IMPL(coro_inst) == xen_globals->implements->coroutine) {
    Xen_Coroutine* coro = (Xen_Coroutine*)coro_inst;
    Xen_Vector_Push((Xen_Instance*)cur_coro->await->ptr, coro_inst);
    Xen_GC_Write_Field(&coro->awaiter, (struct __GC_Header *)cur_coro);
    if (Xen_Coroutine_GStatus(coro_inst) == Xen_CORO_CREATED)
      Xen_EventLoop_Task_Push(Xen_Async_Get_EventLoop(), coro_inst);
  } else {
    Xen_Instance* iter = Xen_Attr_Iter(coro_inst);
    if (iter == NULL) {
      Xen_IterError(coro_inst);
    }
    for (Xen_Instance* coro = NULL; (coro = Xen_Attr_Next(iter)) != NULL;) {
      if (Xen_IMPL(coro) != xen_globals->implements->coroutine) {
        Xen_AsyncError_Impl();
        return 0;
      }
      Xen_Vector_Push((Xen_Instance*)cur_coro->await->ptr, coro);
      Xen_GC_Write_Field(&((Xen_Coroutine*)coro)->awaiter, (struct __GC_Header *)cur_coro);
      if (Xen_Coroutine_GStatus(coro) == Xen_CORO_CREATED)
        Xen_EventLoop_Task_Push(Xen_Async_Get_EventLoop(), coro);
    }
    if (!Xen_VM_Except_Active() ||
        strcmp(((Xen_Except*)(*xen_globals->vm)->except.except->ptr)->type,
               "RangeEnd") != 0) {
      return 0;
    }
    (*xen_globals->vm)->except.active = 0;
  }
  cur_coro->status = Xen_CORO_PAUSE;
  return 1;
}

Xen_Instance* Xen_Coroutine_Await_Resume(Xen_Instance* coro_inst) {
  if (!Xen_Async_Get_Active()) {
    Xen_AsyncError();
    return NULL;
  }
  Xen_Coroutine* coro = (Xen_Coroutine*)coro_inst;
  Xen_Instance* await = (Xen_Instance*)coro->await->ptr;
  Xen_Instance* result = NULL;
  if (coro->awaited_excepted > 0) {
    for (Xen_size_t idx = 0; idx < Xen_SIZE(await); idx++) {
      Xen_Coroutine* awaited = (Xen_Coroutine*)Xen_Vector_Get_Index(await, idx);
      if (awaited->except.active) {
        (*xen_globals->vm)->except.active = 1;
        Xen_GC_Write_Field(&(*xen_globals->vm)->except.except, awaited->except.except->ptr);
        vm_backtrace_copy(awaited->except.bt, (*xen_globals->vm)->except.bt);
        vm_backtrace_clear(awaited->except.bt);
        awaited->except.active = 0;
        return NULL;
      }
    }
  } else if (Xen_SIZE(await) == 1) {
    Xen_Coroutine* awaited = (Xen_Coroutine*)Xen_Vector_Top(await);
    if (awaited->except.active) {
      (*xen_globals->vm)->except.active = 1;
      Xen_GC_Write_Field(&(*xen_globals->vm)->except.except, awaited->except.except->ptr);
      vm_backtrace_copy(awaited->except.bt, (*xen_globals->vm)->except.bt);
      vm_backtrace_clear(awaited->except.bt);
      awaited->except.active = 0;
      return NULL;
    }
    result = (Xen_Instance*)awaited->result->ptr;
  } else {
    result = Xen_Vector_New();
    for (Xen_size_t idx = 0; idx < Xen_SIZE(await); idx++) {
      Xen_Coroutine* awaited = (Xen_Coroutine*)Xen_Vector_Get_Index(await, idx);
      if (awaited->except.active) {
        (*xen_globals->vm)->except.active = 1;
        Xen_GC_Write_Field(&(*xen_globals->vm)->except.except, awaited->except.except->ptr);
        vm_backtrace_copy(awaited->except.bt, (*xen_globals->vm)->except.bt);
        vm_backtrace_clear(awaited->except.bt);
        awaited->except.active = 0;
        return NULL;
      }
      Xen_Vector_Push(result, (Xen_Instance*)awaited->result->ptr);
    }
  }
  Xen_Vector_Clear(await);
  coro->awaited_ready = 0;
  return result;
}

void Xen_Coroutine_Return(Xen_Instance* coro, Xen_Instance* r) {
  Xen_IGC_Write_Field(&((Xen_Coroutine*)coro)->result, r);
  Xen_Coroutine_SStatus(coro, Xen_CORO_TERMINATED);
}

void Xen_Coroutine_Excepted(Xen_Instance* coro_inst) {
  Xen_Coroutine* coro = (Xen_Coroutine*)coro_inst;
  coro->except.active = 1;
  Xen_GC_Write_Field(&coro->except.except, (*xen_globals->vm)->except.except->ptr);
  vm_backtrace_copy((*xen_globals->vm)->except.bt, coro->except.bt);
  vm_backtrace_clear((*xen_globals->vm)->except.bt);
  (*xen_globals->vm)->except.active = 0;
  if (coro->awaiter->ptr) {
    Xen_Coroutine* awaiter = (Xen_Coroutine*)coro->awaiter->ptr;
    awaiter->awaited_excepted++;
  }
  coro->status = Xen_CORO_TERMINATED;
}
