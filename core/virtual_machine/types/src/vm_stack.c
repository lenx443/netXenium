#include <stdlib.h>

#include "gc_header.h"
#include "instance.h"
#include "vm_stack.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_typedefs.h"

static void vm_stack_trace(Xen_GCHeader* h) {
  struct vm_Stack* stack = (struct vm_Stack*)h;
  if (stack->stack_head) {
    for (Xen_size_t i = 0; &stack->stack_head[i] < stack->stack_top; i++) {
      Xen_GC_Trace_GCHeader((Xen_GCHeader*)stack->stack_head[i]);
    }
  }
}

static void vm_stack_free(Xen_GCHeader** h) {
  struct vm_Stack* stack = *(struct vm_Stack**)h;
  if (stack->stack_head) {
    Xen_Dealloc(stack->stack_head);
  }
  vm_stack_start(stack);
  Xen_Dealloc(*h);
}

struct vm_Stack* vm_stack_new(size_t cap) {
  struct vm_Stack* stack = (struct vm_Stack*)Xen_GC_New(
      sizeof(struct vm_Stack), vm_stack_trace, vm_stack_free);
  stack->stack_head = Xen_ZAlloc(cap, sizeof(Xen_Instance*));
  if (!stack->stack_head) {
    return 0;
  }
  stack->stack_top = stack->stack_head;
  stack->stack_capacity = cap;
  return stack;
}
