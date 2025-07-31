#include <stdlib.h>

#include "logs.h"
#include "vm_def.h"
#include "vm_register.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

VM_ptr vm = NULL;

int vm_create() {
  if (vm != NULL) return 1;
  vm = malloc(sizeof(VM));
  if (!vm) {
    error("No hay memoria disponible");
    return 0;
  }
  if (!vm_register_new(&vm->reg)) {
    free(vm);
    return 0;
  }
  vm->String_Table = vm_string_table_new();
  if (!vm->String_Table) {
    vm_register_free(vm->reg);
    free(vm);
    return 0;
  }
  vm->bytecode = bc_new();
  if (!vm->bytecode) {
    vm_string_table_free(vm->String_Table);
    vm_register_free(vm->reg);
    free(vm);
    return 0;
  }
  vm->ip = 0;
  vm->running = 0;
  return 1;
}

void vm_destroy() {
  if (!vm) return;
  bc_free(vm->bytecode);
  vm_string_table_free(vm->String_Table);
  vm_register_free(vm->reg);
  free(vm);
}
