#include "coroutine.h"
#include "coroutine_instance.h"
#include "gc_header.h"
#include "instance.h"
#include "run_ctx.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
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

void Xen_Coroutine_SStatus(Xen_Instance* coro, int status) {
  ((Xen_Coroutine*)coro)->status = status;
}

int Xen_Coroutine_GStatus(Xen_Instance* coro) {
  return ((Xen_Coroutine*)coro)->status;
}

void Xen_Coroutine_Return(Xen_Instance* coro, Xen_Instance* r) {
  Xen_IGC_Write_Field(&((Xen_Coroutine*)coro)->result, r);
  Xen_Coroutine_SStatus(coro, Xen_CORO_TERMINATED);
}
