#ifndef __VM_H__
#define __VM_H__

#include <stddef.h>
#include <stdint.h>

#include "bytecode.h"
#include "vm_string_table.h"

typedef struct {
  uintptr_t *reg;
  size_t capacity;
} VM_Register;

typedef struct {
  vm_String_Table_ptr String_Table;
  VM_Register reg;
  Bytecode_Array_ptr bytecode;
  uint32_t ip;
  int running;
} VM;

typedef VM *VM_ptr;

VM_ptr vm_new();
void vm_run(VM_ptr);
void vm_free(VM_ptr);

#endif
