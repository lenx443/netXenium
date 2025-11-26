#include "callable.h"
#include "bytecode.h"
#include "gc_header.h"
#include "xen_alloc.h"
#include "xen_gc.h"

static void callable_trace(Xen_GCHeader* h) {
  CALLABLE_ptr callable = (CALLABLE_ptr)h;
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)callable->code.consts);
}

static void callable_destroy(Xen_GCHeader** h) {
  CALLABLE_ptr callable = *(CALLABLE_ptr*)h;
  bc_free(callable->code.code);
  Xen_Dealloc(*h);
}

CALLABLE_ptr callable_new(ProgramCode_t bpc) {
  CALLABLE_ptr new_callable = (CALLABLE_ptr)Xen_GC_New(
      sizeof(CALLABLE), callable_trace, callable_destroy);
  if (!new_callable) {
    return NULL;
  }
  new_callable->code = bpc;
  return new_callable;
}
