#include "basic.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_timer_instance.h"
#include "xen_timer_implement.h"

static void timer_trace(Xen_Instance* inst) {
  Xen_Timer* timer = (Xen_Timer*)inst;
  if (timer->coroutine && timer->coroutine->ptr)
    Xen_GC_Trace_GCHeader(timer->coroutine);
}

static Xen_Instance*
timer_alloc(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Timer* timer = (Xen_Timer*)Xen_Instance_Alloc(xen_globals->implements->timer);
  timer->coroutine = Xen_GCHandle_New((Xen_GCHeader*)timer);
  timer->expire = 0;
  timer->index = 0;
  timer->cancelled = 0;
  timer->callback = NULL;
  timer->cb_data = NULL;
  return (Xen_Instance*)timer;
}

static Xen_Instance*
timer_destroy(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_GCHandle_Free(((Xen_Timer*)self)->coroutine);
  return nil;
}

static Xen_Implement __Timer_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "EventLoop",
    .__inst_size = sizeof(struct Xen_Timer_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = timer_trace,
    .__props = NULL,
    .__alloc = timer_alloc,
    .__create = NULL,
    .__destroy = timer_destroy,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = NULL,
};

struct __Implement* Xen_Timer_GetImplement(void) {
  return &__Timer_Implement;
}
