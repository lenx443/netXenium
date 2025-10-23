#ifndef __VM_STACK_H__
#define __VM_STACK_H__

#include <stddef.h>

#include "instance.h"

struct vm_Stack {
  Xen_Instance** stack_head;
  Xen_Instance** stack_top;
  size_t stack_capacity;
};

int vm_stack_init(struct vm_Stack*, size_t);
void vm_stack_free(struct vm_Stack);

static inline void vm_stack_start(struct vm_Stack* stack) {
  stack->stack_head = NULL;
  stack->stack_top = NULL;
  stack->stack_capacity = 0;
}

static inline void vm_stack_push(struct vm_Stack stack, Xen_Instance* val) {
  *stack.stack_top++ = Xen_ADD_REF(val);
}
static inline Xen_Instance* vm_stack_pop(struct vm_Stack stack) {
  Xen_Instance* val = *--stack.stack_top;
  return val;
}

#endif
