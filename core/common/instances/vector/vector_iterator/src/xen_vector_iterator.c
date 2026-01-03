#include "xen_vector_iterator.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_vector_iterator_instance.h"

Xen_Instance* Xen_Vector_Iterator_New(Xen_Instance* vector) {
  Xen_Vector_Iterator* it = (Xen_Vector_Iterator*)__instance_new(
      xen_globals->implements->vector_iterator, nil, nil, 0);
  if (!it) {
    return NULL;
  }
  Xen_GC_Write_Field((Xen_GCHeader*)it, (Xen_GCHandle**)&it->vector,
                     (Xen_GCHeader*)vector);
  it->index = 0;
  return (Xen_Instance*)it;
}
