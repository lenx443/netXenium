#include "coroutine_implement.h"
#include "basic_templates.h"
#include "callable.h"
#include "coroutine_instance.h"
#include "basic.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_vector.h"

static void coroutine_trace(Xen_Instance* inst) {
  Xen_Coroutine* coro = (Xen_Coroutine*)inst;
  if (coro->context && coro->context->ptr)  Xen_GC_Trace_GCHeader(coro->context);
  if (coro->self    && coro->self->ptr)     Xen_GC_Trace_GCHeader(coro->self);
  if (coro->args    && coro->args->ptr)     Xen_GC_Trace_GCHeader(coro->args);
  if (coro->kwargs  && coro->kwargs->ptr)   Xen_GC_Trace_GCHeader(coro->kwargs);
  if (coro->igcfork && coro->igcfork->ptr)  Xen_GC_Trace_GCHeader(coro->igcfork);
  if (coro->await   && coro->await->ptr)    Xen_GC_Trace_GCHeader(coro->await);
  if (coro->awaiter && coro->awaiter->ptr)  Xen_GC_Trace_GCHeader(coro->awaiter);
  if (coro->result  && coro->result->ptr)   Xen_GC_Trace_GCHeader(coro->result);
  if (coro->except.active)                  Xen_GC_Trace_GCHeader(coro->except.except);
}

static Xen_Instance* coroutine_alloc(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Coroutine* coro = (Xen_Coroutine*)Xen_Instance_Alloc(xen_globals->implements->coroutine);
  Xen_IGC_Push((Xen_Instance*)coro);
  coro->context = Xen_GCHandle_New((Xen_GCHeader*)coro);
  coro->self = Xen_GCHandle_New((Xen_GCHeader*)coro);
  coro->args = Xen_GCHandle_New((Xen_GCHeader*)coro);
  coro->kwargs = Xen_GCHandle_New((Xen_GCHeader*)coro);
  coro->igcfork = Xen_GCHandle_New_From((Xen_GCHeader*)coro, (Xen_GCHeader*)Xen_IGC_Fork_New());
  coro->await = Xen_GCHandle_New_From((Xen_GCHeader*)coro, (Xen_GCHeader*)Xen_Vector_New());
  coro->awaiter = Xen_GCHandle_New((Xen_GCHeader*)coro);
  coro->result = Xen_GCHandle_New((Xen_GCHeader*)coro);
  coro->except.except = Xen_GCHandle_New((Xen_GCHeader*)coro);
  coro->except.bt = vm_backtrace_new();
  Xen_IGC_Pop();
  return (Xen_Instance*)coro;
}

static Xen_Instance* coroutine_destroy(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Coroutine* coro = (Xen_Coroutine*)self;
  Xen_GCHandle_Free(coro->context);
  Xen_GCHandle_Free(coro->self);
  Xen_GCHandle_Free(coro->args);
  Xen_GCHandle_Free(coro->kwargs);
  Xen_GCHandle_Free(coro->igcfork);
  Xen_GCHandle_Free(coro->await);
  Xen_GCHandle_Free(coro->awaiter);
  Xen_GCHandle_Free(coro->result);
  Xen_GCHandle_Free(coro->except.except);
  vm_backtrace_free(coro->except.bt);
  if (coro->data) Xen_Dealloc(coro->data);
  return nil;
}

static Xen_Instance* coroutine_status(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Coroutine* coro = (Xen_Coroutine*)self;
  return Xen_Number_From_Int(coro->status);
}

static Xen_Instance* coroutine_excepted(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Coroutine* coro = (Xen_Coroutine*)self;
  return coro->except.active ? Xen_True : Xen_False;
}

static Xen_Instance* coroutine_except_throw(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Coroutine* coro = (Xen_Coroutine*)self;
  if (coro->except.active) {
    (*xen_globals->vm)->except.active = 1;
    Xen_GC_Write_Field(&(*xen_globals->vm)->except.except, coro->except.except->ptr);
    vm_backtrace_copy(coro->except.bt, (*xen_globals->vm)->except.bt);
    vm_backtrace_clear(coro->except.bt);
    coro->except.active = 0;
  }
  return nil;
}

static Xen_Instance* coroutine_result(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Coroutine* coro = (Xen_Coroutine*)self;
  return (Xen_Instance*)coro->result->ptr;
}

struct __Implement __Coroutine_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Coroutine",
    .__inst_size = sizeof(struct Xen_Coroutine_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = coroutine_trace,
    .__props = NULL,
    .__alloc = coroutine_alloc,
    .__create = NULL,
    .__destroy = coroutine_destroy,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
    .__set_attr = NULL,
};

struct __Implement* Xen_Coroutine_GetImplement(void) {
  return &__Coroutine_Implement;
}

int Xen_Coroutine_Init(void) {
  Xen_Instance* props = Xen_Map_New();
  if (!props) {
    return 0;
  }
  Xen_VM_Store_Native_Function(props, "status", coroutine_status, nil);
  Xen_VM_Store_Native_Function(props, "excepted", coroutine_excepted, nil);
  Xen_VM_Store_Native_Function(props, "except_throw", coroutine_except_throw, nil);
  Xen_VM_Store_Native_Function(props, "result", coroutine_result, nil);
  __Coroutine_Implement.__props =
      Xen_GCHandle_New_From((Xen_GCHeader*)impls_maps, (Xen_GCHeader*)props);
  Xen_IGC_Fork_Push(impls_maps, props);
  return 1;
}

void Xen_Coroutine_Finish(void) {
  Xen_GCHandle_Free(__Coroutine_Implement.__props);
}
