#include <stddef.h>
#include <stdlib.h>

#include "GCPointer.h"
#include "gc_pointer_list.h"
#include "logs.h"

#define error(msg, ...) log_add(NULL, ERROR, "GCPointer list", msg, ##__VA_ARGS__)

GCPointer_ptr gc_pointer_list_append(GCPointer_node_ptr *gc_ptr_list, void *ptr,
                                     size_t size) {
  GCPointer_node_ptr new_node = malloc(sizeof(GCPointer_node_t));
  if (!new_node) {
    error("Memoria insuficiente");
    return NULL;
  }
  new_node->gc_pointer = gc_pointer_new(ptr, size);
  if (!new_node->gc_pointer) {
    free(new_node);
    return NULL;
  }
  new_node->next = *gc_ptr_list;
  *gc_ptr_list = new_node;
  return new_node->gc_pointer;
}

void gc_pointer_list_free(GCPointer_node_ptr gc_ptr_list) {
  GCPointer_node_ptr current = gc_ptr_list;
  while (current) {
    GCPointer_node_ptr next = current->next;
    gc_pointer_free(gc_ptr_list->gc_pointer);
    free(current);
    current = next;
  }
}
