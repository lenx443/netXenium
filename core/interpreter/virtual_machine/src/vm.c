#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "GCPointer.h"
#include "bytecode.h"
#include "garbage_collector.h"
#include "gc_pointer_list.h"
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
  GCPointer_node_ptr gc_array = NULL;
  while (vm->running) {
    const bc_Instruct_t instr = vm->bytecode->bc_array[vm->ip++];
    switch (instr.bci_opcode) {
    case OP_NOP: break;
    case OP_SYSCALL: {
      uintptr_t syscall_num = vm->reg.reg[0];
      uintptr_t args[6];
      for (int i = 1; i <= 6; i++) {
        if (vm->reg.point_flag[i]) {
          args[i - 1] = (uintptr_t)((GCPointer_ptr)vm->reg.reg[i])->gc_ptr;
        } else {
          args[i - 1] = vm->reg.reg[i];
        }
      }
      vm->reg.reg[1] =
          syscall(syscall_num, args[0], args[1], args[2], args[3], args[4], args[5]);
      break;
    }
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
    case OP_STRING_CONCAT: {
      if (BC_REG_GET_VALUE(instr.bci_dst) >= vm->reg.capacity) {
        vm->running = 0;
        break;
      }
      if (BC_REG_GET_VALUE(instr.bci_src1) >= vm->reg.capacity) {
        vm->running = 0;
        break;
      }
      if (instr.bci_src2 >= vm->String_Table->size) {
        vm->running = 0;
        break;
      }
      uint8_t src_reg = BC_REG_GET_VALUE(instr.bci_src1);
      char *reg_value;
      if (vm->reg.point_flag[src_reg]) {
        reg_value = (char *)((GCPointer_ptr)vm->reg.reg[src_reg])->gc_ptr;
      } else {
        reg_value = (char *)vm->reg.reg[src_reg];
      }
      char *concat_string = vm->String_Table->strings[instr.bci_src2];
      int str_size = strlen(reg_value) + strlen(concat_string) + 1;
      char buffer[str_size];
      strcpy(buffer, reg_value);
      strcat(buffer, concat_string);
      buffer[str_size - 1] = '\0';
      GCPointer_ptr temp = gc_pointer_list_append(&gc_array, buffer, str_size);
      if (!temp) {
        garbage_collector_run_as_registers(&gc_array, vm);
        temp = gc_pointer_list_append(&gc_array, buffer, str_size);
        if (!temp) break;
      }
      vm->reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (uintptr_t)temp;
      vm->reg.point_flag[BC_REG_GET_VALUE(instr.bci_dst)] = 1;
      break;
    }
    case OP_HALT:
    default: vm->running = 0; break;
    }
  }
  gc_pointer_list_free(gc_array);
}

void vm_free(VM_ptr vm) {
  if (!vm) return;
  bc_free(vm->bytecode);
  vm_string_table_free(vm->String_Table);
  free(vm->reg.reg);
  free(vm->reg.point_flag);
  free(vm);
}
