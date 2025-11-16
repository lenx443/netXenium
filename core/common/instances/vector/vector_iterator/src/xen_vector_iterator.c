#include "xen_vector_iterator.h"
#include "instance.h"
#include "xen_nil.h"
#include "xen_vector_iterator_implement.h"
#include "xen_vector_iterator_instance.h"

Xen_Instance* Xen_Vector_Iterator_New(Xen_Instance* vector) {
  Xen_Vector_Iterator* it = (Xen_Vector_Iterator*)__instance_new(
      &Xen_Vector_Iterator_Implement, nil, nil, 0);
  if (!it) {
    return NULL;
  }
  it->vector = Xen_ADD_REF(vector);
  it->index = 0;
  return (Xen_Instance*)it;
}
