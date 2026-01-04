#ifndef __VM_STACK_H__
#define __VM_STACK_H__

#include <stddef.h>

#include "gc_header.h"
#include "instance.h"
#include "xen_gc.h"

struct vm_Stack {
  Xen_GCHeader gc;
  Xen_GCHandle** stack_head;
  Xen_GCHandle** stack_top;
  size_t stack_capacity;
};

struct vm_Stack* vm_stack_new(size_t);

static inline void vm_stack_start(struct vm_Stack* stack) {
  stack->stack_head = NULL;
  stack->stack_top = NULL;
  stack->stack_capacity = 0;
}

static inline void vm_stack_push(struct vm_Stack* stack, Xen_Instance* val) {
  Xen_GC_Write_Field((Xen_GCHeader*)stack, (Xen_GCHandle**)stack->stack_top++,
                     (Xen_GCHeader*)val);
}
static inline Xen_Instance* vm_stack_pop(struct vm_Stack* stack) {
  Xen_Instance* val = (Xen_Instance*)(*--stack->stack_top)->ptr;
  return val;
}

#endif
