#include "coroutine_implement.h"
#include "callable.h"
#include "coroutine_instance.h"
#include "basic.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"

static void coroutine_trace(Xen_Instance* inst) {
  Xen_Coroutine* coro = (Xen_Coroutine*)inst;
  if (coro->context->ptr) Xen_GC_Trace_GCHeader(coro->context);
}

static Xen_Instance* coroutine_alloc(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Coroutine* coro = (Xen_Coroutine*)Xen_Instance_Alloc(xen_globals->implements->coroutine);
  coro->context = Xen_GCHandle_New((Xen_GCHeader*)coro);
  return (Xen_Instance*)coro;
}

static Xen_Instance* coroutine_destroy(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Coroutine* coro = (Xen_Coroutine*)self;
  Xen_GCHandle_Free(coro->context);
  return nil;
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
    .__get_attr = NULL,
    .__set_attr = NULL,
};

struct __Implement* Xen_Coroutine_GetImplement(void) {
  return &__Coroutine_Implement;
}
