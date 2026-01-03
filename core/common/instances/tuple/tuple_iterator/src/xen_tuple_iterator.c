#include "xen_tuple_iterator.h"
#include "gc_header.h"
#include "instance.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_tuple_iterator_instance.h"

Xen_Instance* Xen_Tuple_Iterator_New(Xen_Instance* tuple) {
  Xen_Tuple_Iterator* it = (Xen_Tuple_Iterator*)__instance_new(
      xen_globals->implements->tuple_iterator, nil, nil, 0);
  if (!it) {
    return NULL;
  }
  Xen_GC_Write_Field((Xen_GCHeader*)it, (Xen_GCHandle**)&it->tuple,
                     (Xen_GCHeader*)tuple);
  it->index = 0;
  return (Xen_Instance*)it;
}
