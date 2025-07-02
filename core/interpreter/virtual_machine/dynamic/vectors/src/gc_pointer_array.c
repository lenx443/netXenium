#include <stddef.h>
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

int gc_pointer_array_add(GCPointer_array_ptr gc_ptr_array, GCPointer_ptr gc_ptr) {
  if (!gc_ptr_array) {
    error("Arreglo vacío");
    return 0;
  }
  if (gc_ptr_array->size >= gc_ptr_array->capacity) {
    int new_capacity = (gc_ptr_array->capacity == 0) ? 8 : gc_ptr_array->capacity * 2;
    GCPointer_ptr *new_mem =
        realloc(gc_ptr_array->gc_pointers, new_capacity * sizeof(GCPointer_t));
    if (!new_mem) {
      error("No se le pudo asignar mas memoria al arreglo");
      return 0;
    }
    gc_ptr_array->gc_pointers = new_mem;
    gc_ptr_array->capacity = new_capacity;
  }
  gc_ptr_array->gc_pointers[gc_ptr_array->size++] = gc_ptr;
  return 1;
}

void gc_pointer_array_free(GCPointer_array_ptr gc_ptr_array) {
  if (!gc_ptr_array) return;
  for (int i = 0; i < gc_ptr_array->size; i++)
    gc_pointer_free(gc_ptr_array->gc_pointers[i]);
  free(gc_ptr_array);
}
