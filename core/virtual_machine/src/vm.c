#include <stddef.h>
#include <stdint.h>
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
#include "run_ctx.h"
#include "vm.h"
#include "vm_register.h"
#include "vm_string_table.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

void vm_ctx_clear(RunContext_ptr ctx) {
  bc_clear(ctx->ctx_code.code);
  vm_string_table_clear(ctx->ctx_code.strings);
  vm_register_clear(&ctx->ctx_reg);
  ctx->ctx_ip = 0;
  ctx->ctx_running = 0;
}

void vm_run_ctx(RunContext_ptr ctx) {
  static void *dispatch_table[] = {&&NOP,         &&SYSCALL,       &&FUN_CALL,
                                   &&JUMP,        &&JUMP_IF_SQUAD, &&LOAD_IMM,
                                   &&LOAD_STRING, &&LOAD_PROP,     &&STRING_CONCAT,
                                   &&REG_CONCAT,  &&HALT};
  ctx->ctx_running = 1;
  GCPointer_node_ptr gc_array = NULL;
  bc_Instruct_t instr;
  while (ctx->ctx_running && ctx->ctx_ip < ctx->ctx_code.code->bc_size &&
         !program.closed) {
    bc_Instruct_t instr = ctx->ctx_code.code->bc_array[ctx->ctx_ip++];
    if (instr.bci_opcode > OP_HALT) {
      ctx->ctx_running = 0;
      break;
    }

    goto *dispatch_table[instr.bci_opcode];
  NOP:
    continue;
  SYSCALL: {
    reg_t syscall_num = ctx->ctx_reg.reg[0];
    reg_t args[6];
    for (int i = 1; i <= 6; i++) {
      if (ctx->ctx_reg.point_flag[i]) {
        args[i - 1] = (reg_t)((GCPointer_ptr)ctx->ctx_reg.reg[i])->gc_ptr;
      } else {
        args[i - 1] = ctx->ctx_reg.reg[i];
      }
    }
    ctx->ctx_reg.reg[1] =
        syscall(syscall_num, args[0], args[1], args[2], args[3], args[4], args[5]);
    continue;
  }
  FUN_CALL: {
    char *fun_name = (char *)ctx->ctx_reg.reg[0];
    const Command *cmd = NULL;
    for (int i = 0; cmds_table[i] != NULL; i++) {
      if (strcmp(cmds_table[i]->name, fun_name) == 0) {
        cmd = cmds_table[i];
        break;
      }
    }
    if (cmd == NULL) {
      error("No se encontro el commando '%s'", fun_name);
      ctx->ctx_running = 0;
      continue;
    }
    int arg_num = instr.bci_src2;
    if (arg_num < cmd->args_len[0] || arg_num > cmd->args_len[1]) {
      error("Numero de argumentos invalido");
      ctx->ctx_running = 0;
      continue;
    }
    LIST_ptr args = list_new();
    if (!list_push_back_string_node(args, fun_name)) {
      error("Memoria insuficiente");
      ctx->ctx_running = 0;
      continue;
    }
    for (int i = 1; i <= arg_num; i++) {
      char *arg_val;
      if (ctx->ctx_reg.point_flag[i]) {
        arg_val = (char *)((GCPointer_ptr)ctx->ctx_reg.reg[i])->gc_ptr;
      } else {
        arg_val = (char *)ctx->ctx_reg.reg[i];
      }
      if (!list_push_back_string_node(args, arg_val)) {
        error("Memoria insuficiente");
        ctx->ctx_running = 0;
        continue;
      }
    }
    int ret = ctx->ctx_reg.reg[1] = cmd->func(args);
    if (ret != EXIT_SUCCESS) {
      if (ret == 153) error("Error interno en '%s'", fun_name);
      list_free(args);
      ctx->ctx_running = 0;
      continue;
    }
    list_free(args);
    continue;
  }
  JUMP: {
    ctx->ctx_ip = instr.bci_src2;
    continue;
  }
  JUMP_IF_SQUAD: {
    char *reg1_value;
    if (ctx->ctx_reg.point_flag[1]) {
      reg1_value = (char *)((GCPointer_ptr)ctx->ctx_reg.reg[1])->gc_ptr;
    } else {
      reg1_value = (char *)ctx->ctx_reg.reg[1];
    }
    char *reg2_value;
    if (ctx->ctx_reg.point_flag[2]) {
      reg2_value = (char *)((GCPointer_ptr)ctx->ctx_reg.reg[2])->gc_ptr;
    } else {
      reg2_value = (char *)ctx->ctx_reg.reg[2];
    }
    if (strcmp(reg1_value, reg2_value) == 0) { ctx->ctx_ip = instr.bci_src2; }
    continue;
  }
  LOAD_IMM:
    if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
      ctx->ctx_running = 0;
      continue;
    }
    ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = instr.bci_src2;
    continue;
  LOAD_STRING:
    if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
      ctx->ctx_running = 0;
      continue;
    }
    if (instr.bci_src2 >= ctx->ctx_code.strings->size) {
      ctx->ctx_running = 0;
      continue;
    }
    ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] =
        (reg_t)ctx->ctx_code.strings->strings[instr.bci_src2];
    continue;
  LOAD_PROP:
    if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
      ctx->ctx_running = 0;
      continue;
    }
    if (instr.bci_src2 >= ctx->ctx_code.strings->size) {
      ctx->ctx_running = 0;
      continue;
    }
    char *prop_key = ctx->ctx_code.strings->strings[instr.bci_src2];
    prop_struct *prop = prop_reg_value(prop_key, *prop_register, 1);
    if (!prop) {
      error("No se encontro la propiedad '%s' no se encontro", prop_key);
      ctx->ctx_running = 0;
      continue;
    }
    ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (reg_t)prop->value;
    continue;
  STRING_CONCAT: {
    if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
      ctx->ctx_running = 0;
      continue;
    }
    if (BC_REG_GET_VALUE(instr.bci_src1) >= ctx->ctx_reg.capacity) {
      ctx->ctx_running = 0;
      continue;
    }
    if (instr.bci_src2 >= ctx->ctx_code.strings->size) {
      ctx->ctx_running = 0;
      continue;
    }
    uint8_t src_reg = BC_REG_GET_VALUE(instr.bci_src1);
    char *reg1_value;
    if (ctx->ctx_reg.point_flag[src_reg]) {
      reg1_value = (char *)((GCPointer_ptr)ctx->ctx_reg.reg[src_reg])->gc_ptr;
    } else {
      reg1_value = (char *)ctx->ctx_reg.reg[src_reg];
    }
    char *reg2_value = ctx->ctx_code.strings->strings[instr.bci_src2];
    int str_size = strlen(reg1_value) + strlen(reg2_value) + 1;
    char buffer[str_size];
    strcpy(buffer, reg1_value);
    strcat(buffer, reg2_value);
    buffer[str_size - 1] = '\0';
    GCPointer_ptr temp = gc_pointer_list_append(&gc_array, buffer, str_size);
    if (!temp) {
      garbage_collector_run_as_registers(&gc_array, ctx);
      temp = gc_pointer_list_append(&gc_array, buffer, str_size);
      if (!temp) {
        error("Memoria insuficiente");
        ctx->ctx_running = 0;
        continue;
      }
    }
    ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (reg_t)temp;
    ctx->ctx_reg.point_flag[BC_REG_GET_VALUE(instr.bci_dst)] = 1;
    continue;
  }
  REG_CONCAT: {
    if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
      ctx->ctx_running = 0;
      continue;
    }
    if (BC_REG_GET_VALUE(instr.bci_src1) >= ctx->ctx_reg.capacity) {
      ctx->ctx_running = 0;
      continue;
    }
    if (BC_REG_GET_VALUE(instr.bci_src2) >= ctx->ctx_reg.capacity) {
      ctx->ctx_running = 0;
      continue;
    }
    uint8_t src1_reg = BC_REG_GET_VALUE(instr.bci_src1);
    uint8_t src2_reg = BC_REG_GET_VALUE(instr.bci_src2);
    char *reg1_value;
    if (ctx->ctx_reg.point_flag[src1_reg]) {
      reg1_value = (char *)((GCPointer_ptr)ctx->ctx_reg.reg[src1_reg])->gc_ptr;
    } else {
      reg1_value = (char *)ctx->ctx_reg.reg[src1_reg];
    }
    char *reg2_value;
    if (ctx->ctx_reg.point_flag[src2_reg]) {
      reg2_value = (char *)((GCPointer_ptr)ctx->ctx_reg.reg[src2_reg])->gc_ptr;
    } else {
      reg2_value = (char *)ctx->ctx_reg.reg[src2_reg];
    }
    int str_size = strlen(reg1_value) + strlen(reg2_value) + 1;
    char *buffer = malloc(str_size);
    if (!buffer) {
      error("memoria insuficiente");
      ctx->ctx_running = 0;
      continue;
    }
    strcpy(buffer, reg1_value);
    strcat(buffer, reg2_value);
    buffer[str_size - 1] = '\0';
    GCPointer_ptr temp = gc_pointer_list_append(&gc_array, buffer, str_size);
    if (!temp) {
      garbage_collector_run_as_registers(&gc_array, ctx);
      temp = gc_pointer_list_append(&gc_array, buffer, str_size);
      if (!temp) {
        error("Memoria insuficiente");
        ctx->ctx_running = 0;
        continue;
      }
    }
    ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (reg_t)temp;
    ctx->ctx_reg.point_flag[BC_REG_GET_VALUE(instr.bci_dst)] = 1;
    continue;
  }
  HALT:
    ctx->ctx_running = 0;
    continue;
  }
  log_show_and_clear(NULL);
  gc_pointer_list_free(gc_array);
}
