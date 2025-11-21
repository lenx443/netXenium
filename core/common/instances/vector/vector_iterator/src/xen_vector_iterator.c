#include "xen_vector_iterator.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_gc.h"
#include "xen_nil.h"
#include "xen_vector_iterator_implement.h"
#include "xen_vector_iterator_instance.h"

Xen_Instance* Xen_Vector_Iterator_New(Xen_Instance* vector) {
  Xen_Vector_Iterator* it = (Xen_Vector_Iterator*)__instance_new(
      &Xen_Vector_Iterator_Implement, nil, nil, 0);
  if (!it) {
    return NULL;
  }
  Xen_GC_Write_Field((Xen_GCHeader*)it, (Xen_GCHeader**)&it->vector,
                     (Xen_GCHeader*)vector);
  it->index = 0;
  return (Xen_Instance*)it;
}
