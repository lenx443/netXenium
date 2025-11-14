#include "xen_tuple_iterator.h"
#include "instance.h"
#include "xen_nil.h"
#include "xen_tuple_iterator_implement.h"
#include "xen_tuple_iterator_instance.h"

Xen_Instance* Xen_Tuple_Iterator_New(Xen_Instance* tuple) {
  Xen_Tuple_Iterator* it = (Xen_Tuple_Iterator*)__instance_new(
      &Xen_Tuple_Iterator_Implement, nil, nil, 0);
  if (!it) {
    return NULL;
  }
  it->tuple = Xen_ADD_REF(tuple);
  it->index = 0;
  return (Xen_Instance*)it;
}
