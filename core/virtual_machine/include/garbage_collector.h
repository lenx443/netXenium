#ifndef __GARBAGE_COLLECTOR_H__
#define __GARBAGE_COLLECTOR_H__

#include "GCPointer.h"
#include "gc_pointer_list.h"
#include "run_ctx.h"

void garbage_collector_mark(GCPointer_ptr);
void garbage_collector_mark_array_as_registers(RunContext_ptr);
void garbage_collector_sweep_array(GCPointer_node_ptr *);
void garbage_collector_run_as_registers(GCPointer_node_ptr *, RunContext_ptr);

#endif
