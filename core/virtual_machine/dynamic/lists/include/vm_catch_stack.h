#ifndef __VM_CATCH_STACK_H__
#define __VM_CATCH_STACK_H__

#include "gc_header.h"
#include "xen_typedefs.h"

struct VM_Catch_Stack {
  Xen_ulong_t handler_offset;
  Xen_c_string_t except_type;
  Xen_GCHandle** stack_top_before_try;
  struct VM_Catch_Stack* next;
};

void vm_catch_stack_push(struct VM_Catch_Stack**, Xen_ulong_t, Xen_c_string_t,
                         Xen_GCHandle**);
struct VM_Catch_Stack* vm_catch_stack_pop(struct VM_Catch_Stack**);
void vm_catch_stack_clear(struct VM_Catch_Stack**);

#endif
