#include "callable.h"
#include "bytecode.h"
#include "gc_header.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_typedefs.h"

static void callable_trace(Xen_GCHeader* h) {
  CALLABLE_ptr callable = (CALLABLE_ptr)h;
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)callable->code.consts);
}

static void callable_destroy(Xen_GCHeader* h) {
  CALLABLE_ptr callable = (CALLABLE_ptr)h;
  bc_free(callable->code.code);
  Xen_Dealloc(h);
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

static void callable_vector_trace(Xen_GCHeader* h) {
  CALLABLE_Vector_ptr cv = (CALLABLE_Vector_ptr)h;
  for (Xen_size_t i = 0; i < cv->count; i++) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)cv->callables[i]);
  }
}

static void callable_vector_destroy(Xen_GCHeader* h) {
  CALLABLE_Vector_ptr cv = (CALLABLE_Vector_ptr)h;
  for (Xen_size_t i = 0; i < cv->count; i++) {
    Xen_GCHandle_Free(cv->callables[i]);
  }
  Xen_Dealloc(cv->callables);
  Xen_Dealloc(h);
}

CALLABLE_Vector_ptr callable_vector_new(void) {
  CALLABLE_Vector_ptr cv = (CALLABLE_Vector_ptr)Xen_GC_New(
      sizeof(CALLABLE_Vector), callable_vector_trace, callable_vector_destroy);
  cv->callables = NULL;
  cv->count = 0;
  cv->cap = 0;
  return cv;
}

void callable_vector_push(CALLABLE_Vector_ptr cv, CALLABLE_ptr callable) {
  assert(cv && callable);
  if (cv->count >= cv->cap) {
    Xen_size_t new_cap = cv->cap ? cv->cap * 2 : 4;
    cv->callables = Xen_Realloc(cv->callables, new_cap * sizeof(Xen_GCHandle*));
    cv->cap = new_cap;
  }
  cv->callables[cv->count] = Xen_GCHandle_New();
  Xen_GC_Write_Field((Xen_GCHeader*)cv,
                     (Xen_GCHandle**)&cv->callables[cv->count++],
                     (Xen_GCHeader*)callable);
}

CALLABLE_ptr callable_vector_get(CALLABLE_Vector_ptr cv, Xen_size_t idx) {
  if (idx >= cv->count) {
    return NULL;
  }
  return (CALLABLE_ptr)cv->callables[idx]->ptr;
}
