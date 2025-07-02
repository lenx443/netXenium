#include <stdlib.h>

#include "GCPointer.h"
#include "gc_pointer_array.h"
#include "logs.h"

#define error(msg, ...)                                                                  \
  log_add(NULL, ERROR, "Garbage Collector Pointer Array", msg, ##__VA_ARGS__)

GCPointer_array_ptr gc_pointer_array_new() {
  GCPointer_array_ptr gc_ptr_array = malloc(sizeof(GCPointer_array_t));
  if (!gc_ptr_array) {
    error("Memoria insuficiente");
    return NULL;
  }
  gc_ptr_array->gc_pointers = NULL;
  gc_ptr_array->size = 0;
  gc_ptr_array->capacity = 0;
  return gc_ptr_array;
}

void gc_pointer_array_free(GCPointer_array_ptr gc_ptr_array) {
  if (!gc_ptr_array) return;
  for (int i = 0; i < gc_ptr_array->size; i++)
    gc_pointer_free(gc_ptr_array->gc_pointers[i]);
  free(gc_ptr_array);
}
