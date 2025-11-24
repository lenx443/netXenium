#include "callable.h"
#include "bytecode.h"
#include "gc_header.h"
#include "vm_consts.h"
#include "xen_alloc.h"
#include "xen_gc.h"

static void callable_trace(Xen_GCHeader* h) {
  CALLABLE_ptr callable = (CALLABLE_ptr)h;
  if (callable->callable_type == CALL_BYTECODE_PROGRAM) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)callable->code.consts->c_names);
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)callable->code.consts->c_instances);
  }
}

static void callable_destroy(Xen_GCHeader** h) {
  CALLABLE_ptr callable = *(CALLABLE_ptr*)h;
  if (callable->callable_type == CALL_BYTECODE_PROGRAM) {
    bc_free(callable->code.code);
    vm_consts_free(callable->code.consts);
  }
  Xen_Dealloc(*h);
}

CALLABLE_ptr callable_new_native(Xen_Native_Func native) {
  CALLABLE_ptr new_callable = (CALLABLE_ptr)Xen_GC_New(
      sizeof(CALLABLE), callable_trace, callable_destroy);
  if (!new_callable) {
    return NULL;
  }
  new_callable->callable_type = CALL_NATIVE_FUNCTIIN;
  new_callable->native_callable = native;
  return new_callable;
}

CALLABLE_ptr callable_new_code(ProgramCode_t bpc) {
  CALLABLE_ptr new_callable = Xen_Alloc(sizeof(CALLABLE));
  if (!new_callable) {
    return NULL;
  }
  new_callable->callable_type = CALL_BYTECODE_PROGRAM;
  new_callable->code = bpc;
  return new_callable;
}
