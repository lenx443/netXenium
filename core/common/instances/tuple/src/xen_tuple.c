#include <stddef.h>
#include <stdlib.h>

#include "instance.h"
#include "xen_nil.h"
#include "xen_tuple.h"
#include "xen_tuple_implement.h"
#include "xen_tuple_instance.h"

Xen_Instance* Xen_Tuple_From_Array(size_t size, Xen_Instance** array) {
  if (!array) {
    return NULL;
  }
  Xen_Tuple* tuple = (Xen_Tuple*)__instance_new(&Xen_Tuple_Implement, nil, 0);
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
