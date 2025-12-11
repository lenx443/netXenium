#include <stddef.h>
#include <stdlib.h>

#include "gc_header.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_nil.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"
#include "xen_vector_instance.h"

Xen_Instance* Xen_Vector_New(void) {
  Xen_Instance* vector = __instance_new(&Xen_Vector_Implement, nil, nil, 0);
  if (!vector)
    return NULL;
  return vector;
}

Xen_Instance* Xen_Vector_From_Array(size_t size, Xen_Instance** array) {
  if (!array) {
    return NULL;
  }
  Xen_INSTANCE* vector = __instance_new(&Xen_Vector_Implement, nil, nil, 0);
  if (!vector) {
    return NULL;
  }
  for (size_t i = 0; i < size; i++) {
    if (!Xen_Vector_Push(vector, array[i])) {
      return NULL;
    }
  }
  return vector;
}

int Xen_Vector_Push(Xen_Instance* vector_inst, Xen_Instance* value) {
  if (!vector_inst || !value) {
    return 0;
  };
  Xen_Vector* vector = (Xen_Vector*)vector_inst;
  if (vector->__size >= vector->capacity) {
    size_t new_cap = vector->capacity == 0 ? 4 : vector->capacity * 2;
    Xen_Instance** new_mem = (Xen_INSTANCE**)Xen_Realloc(
        vector->values, sizeof(Xen_Instance*) * new_cap);
    if (!new_mem) {
      return 0;
    }
    vector->values = new_mem;
    vector->capacity = new_cap;
  }
  Xen_GC_Write_Field((Xen_GCHeader*)vector,
                     (Xen_GCHeader**)&vector->values[vector->__size++],
                     (Xen_GCHeader*)value);
  return 1;
}

int Xen_Vector_Push_Vector(Xen_Instance* vector_dst, Xen_Instance* vector_src) {
  if (!vector_dst || !vector_dst) {
    return 0;
  }
  for (size_t i = 0; i < Xen_SIZE(vector_src); i++) {
    if (!Xen_Vector_Push(vector_dst, Xen_Vector_Peek_Index(vector_src, i))) {
      return 0;
    }
  }
  return 1;
}

Xen_Instance* Xen_Vector_Pop(Xen_Instance* vector_inst) {
  if (!vector_inst) {
    return NULL;
  }
  Xen_Vector* vector = (Xen_Vector*)vector_inst;
  if (Xen_SIZE(vector) == 0) {
    return NULL;
  }
  vector->__size--;
  return vector->values[Xen_SIZE(vector)];
}

Xen_Instance* Xen_Vector_Top(Xen_Instance* vector_inst) {
  if (!vector_inst) {
    return NULL;
  }
  Xen_Vector* vector = (Xen_Vector*)vector_inst;
  if (Xen_SIZE(vector) == 0) {
    return NULL;
  }
  return vector->values[Xen_SIZE(vector) - 1];
}

Xen_Instance* Xen_Vector_Get_Index(Xen_Instance* vector, size_t index) {
  if (!vector || index >= ((Xen_Vector*)vector)->__size) {
    return NULL;
  }
  return ((Xen_Vector*)vector)->values[index];
}

Xen_Instance* Xen_Vector_Peek_Index(Xen_Instance* vector, size_t index) {
  if (!vector || index >= ((Xen_Vector*)vector)->__size) {
    return NULL;
  }
  return ((Xen_Vector*)vector)->values[index];
}

size_t Xen_Vector_Size(Xen_Instance* vector) {
  if (!vector) {
    return 0;
  }
  return ((Xen_Vector*)vector)->__size;
}
