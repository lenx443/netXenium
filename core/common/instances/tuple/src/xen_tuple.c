#include <stddef.h>
#include <stdlib.h>

#include "gc_header.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_tuple.h"
#include "xen_tuple_instance.h"
#include "xen_typedefs.h"
#include "xen_vector_instance.h"

Xen_bool_t Xen_IsTuple(Xen_Instance* inst) {
  return Xen_IMPL(inst) == xen_globals->implements->tuple;
}

Xen_Instance* Xen_Tuple_From_Array(Xen_size_t size, Xen_Instance** array) {
  if (!array) {
    return NULL;
  }
  Xen_Tuple* tuple =
      (Xen_Tuple*)__instance_new(xen_globals->implements->tuple, nil, nil, 0);
  if (!tuple) {
    return NULL;
  }
  tuple->instances = Xen_Alloc(size * sizeof(Xen_Instance*));
  if (!tuple->instances) {
    return NULL;
  }
  for (size_t i = 0; i < size; i++) {
    tuple->instances[tuple->__size] = Xen_GCHandle_New((Xen_GCHeader*)tuple);
    Xen_GC_Write_Field((Xen_GCHeader*)tuple,
                       (Xen_GCHandle**)&tuple->instances[tuple->__size++],
                       (Xen_GCHeader*)array[i]);
  }
  return (Xen_Instance*)tuple;
}

Xen_Instance* Xen_Tuple_From_Vector(Xen_Instance* vector_inst) {
  if (Xen_IMPL(vector_inst) != xen_globals->implements->vector) {
    return NULL;
  }
  Xen_Vector* vector = (Xen_Vector*)vector_inst;
  Xen_Instance** array = Xen_Alloc(Xen_SIZE(vector) * sizeof(Xen_Instance*));
  for (Xen_size_t i = 0; i < Xen_SIZE(vector); i++) {
    array[i] = (Xen_Instance*)vector->values[i]->ptr;
  }
  return Xen_Tuple_From_Array(Xen_SIZE(vector), array);
}

Xen_Instance* Xen_Tuple_Get_Index(Xen_Instance* tuple, Xen_size_t index) {
  if (!tuple || index >= ((Xen_Tuple*)tuple)->__size) {
    return NULL;
  }
  return (Xen_Instance*)((Xen_Tuple*)tuple)->instances[index]->ptr;
}

Xen_Instance* Xen_Tuple_Peek_Index(Xen_Instance* tuple, Xen_size_t index) {
  if (!tuple || index >= ((Xen_Tuple*)tuple)->__size) {
    return NULL;
  }
  return (Xen_Instance*)((Xen_Tuple*)tuple)->instances[index]->ptr;
}
