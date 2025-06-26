#include <stddef.h>
#include <stdlib.h>

#include "bytecode.h"
#include "logs.h"
#include "vm.h"
#include "vm_string_table.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

VM_ptr vm_new() {
  VM_ptr vm = malloc(sizeof(VM));
  if (!vm) {
    error("No hay memoria disponible");
    return NULL;
  }
  size_t reg_capacity = 128;
  vm->reg.reg = malloc(reg_capacity * sizeof(int));
  if (!vm->reg.reg) {
    error("No hay memoria disponible");
    free(vm);
    return NULL;
  }
  vm->reg.capacity = reg_capacity;
  vm->String_Table = vm_string_table_new();
  if (!vm->String_Table) {
    free(vm->reg.reg);
    free(vm);
    return NULL;
  }
  vm->bytecode = bc_new();
  if (!vm->bytecode) {
    vm_string_table_free(vm->String_Table);
    free(vm->reg.reg);
    free(vm);
    return NULL;
  }
  vm->ip = 0;
  vm->running = 0;
  return vm;
}

void vm_free(VM_ptr vm) {
  if (!vm) return;
  bc_free(vm->bytecode);
  vm_string_table_free(vm->String_Table);
  free(vm->reg.reg);
  free(vm);
}
