#ifndef __VM_DEF_H__
#define __VM_DEF_H__

#include "bytecode.h"
#include "vm_register.h"
#include "vm_string_table.h"

typedef struct {
  vm_String_Table_ptr String_Table;
  VM_Register reg;
  Bytecode_Array_ptr bytecode;
  uint32_t ip;
  int running;
} VM;

typedef VM *VM_ptr;

int vm_create();
void vm_destroy();

extern VM_ptr vm;

#endif
