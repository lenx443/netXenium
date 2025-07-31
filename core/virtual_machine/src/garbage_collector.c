#include <stdlib.h>

#include "GCPointer.h"
#include "garbage_collector.h"
#include "gc_pointer_list.h"

void garbage_collector_mark(GCPointer_ptr gc_ptr) {
  if (!gc_ptr || gc_ptr->gc_marked) return;
  gc_ptr->gc_marked = 1;
}

void garbage_collector_mark_array_as_registers(VM_ptr vm) {
  if (!vm) return;
  for (int i = 0; i < vm->reg.capacity; i++) {
    if (vm->reg.point_flag[i]) garbage_collector_mark((GCPointer_ptr)vm->reg.reg[i]);
  }
}

void garbage_collector_sweep_array(GCPointer_node_ptr *gc_ptr_list) {
  if (!gc_ptr_list) return;
  GCPointer_node_ptr current = *gc_ptr_list;
  GCPointer_node_ptr previous = NULL;
  while (current) {
    GCPointer_node_ptr next = current->next;
    if (!current->gc_pointer->gc_marked) {
      gc_pointer_free(current->gc_pointer);
      free(current);
      if (previous)
        previous->next = next;
      else
        *gc_ptr_list = next;
    } else
      previous = current;
    current = next;
  }
}

void garbage_collector_run_as_registers(GCPointer_node_ptr *gc_ptr_list, VM_ptr vm) {
  if (!gc_ptr_list || !vm) return;
  garbage_collector_mark_array_as_registers(vm);
  garbage_collector_sweep_array(gc_ptr_list);
}
