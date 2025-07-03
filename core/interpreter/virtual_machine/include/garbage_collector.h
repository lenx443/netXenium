#ifndef __GARBAGE_COLLECTOR_H__
#define __GARBAGE_COLLECTOR_H__

#include "GCPointer.h"
#include "gc_pointer_list.h"
#include "vm.h"

void garbage_collector_mark(GCPointer_ptr);
void garbage_collector_mark_array_as_registers(VM_ptr);
void garbage_collector_sweep_array(GCPointer_node_ptr *);
void garbage_collector_run_as_registers(GCPointer_node_ptr *, VM_ptr);

#endif
