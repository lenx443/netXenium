#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "GCPointer.h"
#include "bc_instruct.h"
#include "bytecode.h"
#include "garbage_collector.h"
#include "gc_pointer_list.h"
#include "list.h"
#include "logs.h"
#include "program.h"
#include "properties.h"
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
      reg_t syscall_num = vm->reg.reg[0];
      reg_t args[6];
      for (int i = 1; i <= 6; i++) {
        if (vm->reg.point_flag[i]) {
          args[i - 1] = (reg_t)((GCPointer_ptr)vm->reg.reg[i])->gc_ptr;
        } else {
          args[i - 1] = vm->reg.reg[i];
        }
      }
      vm->reg.reg[1] =
          syscall(syscall_num, args[0], args[1], args[2], args[3], args[4], args[5]);
      break;
    }
    case OP_FUN_CALL: {
      char *fun_name = (char *)vm->reg.reg[0];
      const Command *cmd = NULL;
      for (int i = 0; cmds_table[i] != NULL; i++) {
        if (strcmp(cmds_table[i]->name, fun_name) == 0) {
          cmd = cmds_table[i];
          break;
        }
      }
      if (cmd == NULL) {
        error("No se encontro el commando '%s'", fun_name);
        vm->running = 0;
        break;
      }
      int arg_num = instr.bci_src2;
      if (arg_num < cmd->args_len[0] || arg_num > cmd->args_len[1]) {
        error("Numero de argumentos invalido");
        vm->running = 0;
        break;
      }
      LIST_ptr args = list_new();
      if (!list_push_back_string_node(args, fun_name)) {
        error("Memoria insuficiente");
        vm->running = 0;
        break;
      }
      for (int i = 1; i <= arg_num; i++) {
        char *arg_val;
        if (vm->reg.point_flag[i]) {
          arg_val = (char *)((GCPointer_ptr)vm->reg.reg[i])->gc_ptr;
        } else {
          arg_val = (char *)vm->reg.reg[i];
        }
        if (!list_push_back_string_node(args, arg_val)) {
          error("Memoria insuficiente");
          vm->running = 0;
          break;
        }
      }
      vm->reg.reg[1] = cmd->func(args);
      list_free(args);
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
          (reg_t)vm->String_Table->strings[instr.bci_src2];
      break;
    case OP_LOAD_PROP:
      if (BC_REG_GET_VALUE(instr.bci_dst) >= vm->reg.capacity) {
        vm->running = 0;
        break;
      }
      if (instr.bci_src2 >= vm->String_Table->size) {
        vm->running = 0;
        break;
      }
      char *prop_key = vm->String_Table->strings[instr.bci_src2];
      prop_struct *prop = prop_reg_value(prop_key, *prop_register);
      if (!prop) {
        error("No se encontro la propiedad '%s' no se encontro", prop_key);
        vm->running = 0;
        break;
      }
      vm->reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (reg_t)prop->value;
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
        if (!temp) {
          error("Memoria insuficiente");
          vm->running = 0;
          break;
        }
      }
      vm->reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (reg_t)temp;
      vm->reg.point_flag[BC_REG_GET_VALUE(instr.bci_dst)] = 1;
      break;
    }
    case OP_REG_CONCAT: {
      if (BC_REG_GET_VALUE(instr.bci_dst) >= vm->reg.capacity) {
        vm->running = 0;
        break;
      }
      if (BC_REG_GET_VALUE(instr.bci_src1) >= vm->reg.capacity) {
        vm->running = 0;
        break;
      }
      if (BC_REG_GET_VALUE(instr.bci_src2) >= vm->reg.capacity) {
        vm->running = 0;
        break;
      }
      uint8_t src1_reg = BC_REG_GET_VALUE(instr.bci_src1);
      uint8_t src2_reg = BC_REG_GET_VALUE(instr.bci_src2);
      char *reg_value;
      if (vm->reg.point_flag[src1_reg]) {
        reg_value = (char *)((GCPointer_ptr)vm->reg.reg[src1_reg])->gc_ptr;
      } else {
        reg_value = (char *)vm->reg.reg[src1_reg];
      }
      char *concat_string;
      if (vm->reg.point_flag[src2_reg]) {
        concat_string = (char *)((GCPointer_ptr)vm->reg.reg[src2_reg])->gc_ptr;
      } else {
        concat_string = (char *)vm->reg.reg[src2_reg];
      }
      int str_size = strlen(reg_value) + strlen(concat_string) + 1;
      char buffer[str_size];
      strcpy(buffer, reg_value);
      strcat(buffer, concat_string);
      buffer[str_size - 1] = '\0';
      GCPointer_ptr temp = gc_pointer_list_append(&gc_array, buffer, str_size);
      if (!temp) {
        garbage_collector_run_as_registers(&gc_array, vm);
        temp = gc_pointer_list_append(&gc_array, buffer, str_size);
        if (!temp) {
          error("Memoria insuficiente");
          vm->running = 0;
          break;
        }
      }
      vm->reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (reg_t)temp;
      vm->reg.point_flag[BC_REG_GET_VALUE(instr.bci_dst)] = 1;
      break;
    }
    case OP_HALT:
    default: vm->running = 0; break;
    }
  }
  log_show_and_clear(NULL);
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
