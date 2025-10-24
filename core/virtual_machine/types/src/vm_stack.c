#include <stdlib.h>

#include "instance.h"
#include "vm_stack.h"

int vm_stack_init(struct vm_Stack* stack, size_t cap) {
  if (stack != NULL) {
    vm_stack_free(stack);
  }
  stack->stack_head = calloc(cap, sizeof(Xen_Instance*));
  if (!stack->stack_head) {
    return 0;
  }
  stack->stack_top = stack->stack_head;
  stack->stack_capacity = cap;
  return 1;
}

void vm_stack_free(struct vm_Stack* stack) {
  if (stack->stack_head) {
    while (stack->stack_top > stack->stack_head) {
      Xen_DEL_REF(*--stack->stack_top);
    }
    free(stack->stack_head);
  }
  vm_stack_start(stack);
}
