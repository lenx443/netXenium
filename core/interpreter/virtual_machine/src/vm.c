#include <stddef.h>
#include <stdint.h>
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
    error("no hay memoria disponible");
    free(vm);
    return NULL;
  }
  vm->reg.point_flag = malloc(reg_capacity);
  if (!vm->reg.point_flag) {
    error("no hay memoria disponible");
    free(vm);
    return NULL;
  }
  vm->reg.capacity = reg_capacity;
  vm->String_Table = vm_string_table_new();
  if (!vm->String_Table) {
    free(vm->reg.reg);
    free(vm->reg.point_flag);
    free(vm);
    return NULL;
  }
  vm->bytecode = bc_new();
  if (!vm->bytecode) {
    vm_string_table_free(vm->String_Table);
    free(vm->reg.reg);
    free(vm->reg.point_flag);
    free(vm);
    return NULL;
  }
  vm->ip = 0;
  vm->running = 0;
  return vm;
}

void vm_run(VM_ptr vm) {
  vm->running = 1;
  while (vm->running) {
    const bc_Instruct_t instr = vm->bytecode->bc_array[vm->ip++];
    switch (instr.bci_opcode) {
    case OP_NOP: break;
    case OP_LOAD_IMM:
      if (BC_REG_GET_VALUE(instr.bci_dst) >= vm->reg.capacity) {
        vm->running = 0;
        break;
      }
      vm->reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = instr.bci_src2;
      break;
    case OP_LOAD_STRING:
      if (BC_REG_GET_VALUE(instr.bci_dst) >= vm->reg.capacity) {
        vm->running = 0;
        break;
      }
      if (instr.bci_src2 >= vm->String_Table->size) {
        vm->running = 0;
        break;
      }
      vm->reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] =
          (uintptr_t)vm->String_Table->strings[instr.bci_src2];
      break;
    case OP_HALT:
    default: vm->running = 0; break;
    }
  }
}

void vm_free(VM_ptr vm) {
  if (!vm) return;
  bc_free(vm->bytecode);
  vm_string_table_free(vm->String_Table);
  free(vm->reg.reg);
  free(vm->reg.point_flag);
  free(vm);
}
