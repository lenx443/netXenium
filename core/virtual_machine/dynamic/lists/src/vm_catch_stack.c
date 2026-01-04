#include "vm_catch_stack.h"
#include "xen_alloc.h"

void vm_catch_stack_push(struct VM_Catch_Stack** stack,
                         Xen_ulong_t handler_offset, Xen_c_string_t except_type,
                         Xen_GCHandle** stack_top) {
  struct VM_Catch_Stack* new_node = Xen_Alloc(sizeof(struct VM_Catch_Stack));
  new_node->handler_offset = handler_offset;
  new_node->except_type = except_type;
  new_node->stack_top_before_try = stack_top;
  new_node->next = *stack;
  *stack = new_node;
}

struct VM_Catch_Stack* vm_catch_stack_pop(struct VM_Catch_Stack** stack) {
  if (!*stack) {
    return NULL;
  }
  struct VM_Catch_Stack* node = *stack;
  *stack = node->next;
  node->next = NULL;
  return node;
}

void vm_catch_stack_clear(struct VM_Catch_Stack** stack) {
  struct VM_Catch_Stack* current = *stack;
  while (current) {
    struct VM_Catch_Stack* next = current->next;
    Xen_Dealloc((void*)current->except_type);
    Xen_Dealloc(current);
    current = next;
  }
  *stack = NULL;
}
