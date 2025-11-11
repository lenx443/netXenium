#include <stddef.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_nil.h"
#include "xen_tuple.h"
#include "xen_tuple_implement.h"
#include "xen_tuple_instance.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Tuple_From_Array(Xen_size_t size, Xen_Instance** array) {
  if (!array) {
    return NULL;
  }
  Xen_Tuple* tuple =
      (Xen_Tuple*)__instance_new(&Xen_Tuple_Implement, nil, nil, 0);
  if (!tuple) {
    return NULL;
  }
  tuple->instances = malloc(size * sizeof(Xen_Instance*));
  if (!tuple->instances) {
    __instance_free((Xen_Instance*)tuple);
    return NULL;
  }
  for (size_t i = 0; i < size; i++) {
    tuple->instances[tuple->__size++] = Xen_ADD_REF(array[i]);
  }
  return (Xen_Instance*)tuple;
}

Xen_Instance* Xen_Tuple_Get_Index(Xen_Instance* tuple, Xen_size_t index) {
  if (!tuple || index >= ((Xen_Tuple*)tuple)->__size) {
    return NULL;
  }
  return Xen_ADD_REF(((Xen_Tuple*)tuple)->instances[index]);
}

Xen_Instance* Xen_Tuple_Peek_Index(Xen_Instance* tuple, Xen_size_t index) {
  if (!tuple || index >= ((Xen_Tuple*)tuple)->__size) {
    return NULL;
  }
  return ((Xen_Tuple*)tuple)->instances[index];
}
