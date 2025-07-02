#include <stdlib.h>

#include "gc_pointer_array.h"

GCPointer_array_ptr gc_pointer_array_new() {
  GCPointer_array_ptr ptr_array = malloc(sizeof(GCPointer_array_t));
  return ptr_array;
}
