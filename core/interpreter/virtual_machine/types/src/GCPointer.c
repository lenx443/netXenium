#include <stdlib.h>

#include "GCPointer.h"
#include "logs.h"

#define error(msg, ...)                                                                  \
  log_add(NULL, ERROR, "Garbage Collector Pointer", msg, ##__VA_ARGS__)

GCPointer_ptr gc_pointer_new(void *ptr, size_t size) {
  GCPointer_ptr gc_ptr = malloc(sizeof(GCPointer_t));
  if (!gc_ptr) {
    error("Memoria insuficiente");
    return NULL;
  }
  gc_ptr->gc_ptr = malloc(size);
  if (!gc_ptr->gc_ptr) {
    error("Memoria insuficiente");
    free(gc_ptr);
    return NULL;
  }
  memcpy(gc_ptr->gc_ptr, ptr, size);
  gc_ptr->gc_size = size;
  gc_ptr->gc_marked = 0;
  return gc_ptr;
}

void gc_pointer_free(GCPointer_ptr gc_ptr) {
  if (!gc_ptr) return;
  free(gc_ptr->gc_ptr);
  free(gc_ptr);
}
