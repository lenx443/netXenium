#include "xen_eventloop_implement.h"
#include "callable.h"
#include "gc_header.h"
#include "xen_eventloop_instance.h"
#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_timer_heap.h"

#include <unistd.h>

static void eventloop_trace(Xen_Instance* inst) {
  Xen_EventLoop* eloop = (Xen_EventLoop*)inst;
  if (eloop->tasks->ptr) Xen_GC_Trace_GCHeader(eloop->tasks);
  if (eloop->resumed->ptr) Xen_GC_Trace_GCHeader(eloop->resumed);
  if (eloop->timer_heap->ptr) Xen_GC_Trace_GCHeader(eloop->timer_heap);
  if (eloop->cb_interrupt->ptr) Xen_GC_Trace_GCHeader(eloop->cb_interrupt);
}

static Xen_Instance* eventloop_alloc(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_EventLoop* eloop = (Xen_EventLoop*)Xen_Instance_Alloc(xen_globals->implements->eventloop);
  eloop->tasks = Xen_GCHandle_New((Xen_GCHeader*)eloop);
  eloop->resumed = Xen_GCHandle_New((Xen_GCHeader*)eloop);
  eloop->timer_heap = Xen_GCHandle_New_From((Xen_GCHeader*)eloop,
                                            (Xen_GCHeader*)Xen_Timer_Heap_New());
  eloop->cb_interrupt = Xen_GCHandle_New((Xen_GCHeader*)eloop);
  eloop->event_fd = -1;
  eloop->timer_fd = -1;
  eloop->io_refs = 0;
  return (Xen_Instance*)eloop;
}

static Xen_Instance* eventloop_destroy(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_EventLoop* eloop = (Xen_EventLoop*)self;
  Xen_GCHandle_Free(eloop->tasks);
  Xen_GCHandle_Free(eloop->resumed);
  Xen_GCHandle_Free(eloop->timer_heap);
  Xen_GCHandle_Free(eloop->cb_interrupt);
  if (eloop->timer_fd != -1) close(eloop->timer_fd);
  if (eloop->event_fd != -1) close(eloop->event_fd);
  return nil;
}

Xen_Implement __EventLoop_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "EventLoop",
    .__inst_size = sizeof(struct Xen_EventLoop_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = eventloop_trace,
    .__props = NULL,
    .__alloc = eventloop_alloc,
    .__create = NULL,
    .__destroy = eventloop_destroy,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = NULL,
};

struct __Implement* Xen_EventLoop_GetImplement(void) {
  return &__EventLoop_Implement;
}
