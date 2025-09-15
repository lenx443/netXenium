#include <stdlib.h>

#include "bc_instruct.h"
#include "ir_bytecode.h"
#include "ir_instruct.h"
#include "logs.h"

#define error(msg, ...) log_add(NULL, ERROR, "IR Array", msg, ##__VA_ARGS__)

IR_Bytecode_Array_ptr ir_new() {
  IR_Bytecode_Array_ptr intr_array = malloc(sizeof(IR_Bytecode_Array_t));
  if (!intr_array) {
    error("No hay memoria disponible");
    return NULL;
  }
  intr_array->ir_array = NULL;
  intr_array->ir_size = 0;
  intr_array->ir_capacity = 0;
  return intr_array;
}

void ir_free(const IR_Bytecode_Array_ptr ir) {
  if (!ir) return;
  free(ir->ir_array);
  free(ir);
}

int ir_add_instr(IR_Bytecode_Array_ptr ir, IR_Instruct_t instr) {
  if (!ir) {
    error("El arreglo de bytecode esta vacío");
    return 0;
  }
  if (ir->ir_size >= ir->ir_capacity) {
    int new_capacity = (ir->ir_capacity == 0) ? 8 : ir->ir_capacity * 2;
    IR_Instruct_ptr new_mem = realloc(ir->ir_array, new_capacity * sizeof(IR_Instruct_t));
    if (!new_mem) {
      error("No se le pudo asignar mas memoria al arreglo de bytecode");
      return 0;
    }
    ir->ir_array = new_mem;
    ir->ir_capacity = new_capacity;
  }
  ir->ir_array[ir->ir_size++] = instr;
  return 1;
}

int ir_add_nop(IR_Bytecode_Array_ptr ir) {
  return ir_add_instr(ir, (IR_Instruct_t){
                              OP_NOP,
                              0,
                              0,
                              0,
                              NULL,
                          });
}
int ir_add_syscall(IR_Bytecode_Array_ptr ir) {
  return ir_add_instr(ir, (IR_Instruct_t){
                              OP_SYSCALL,
                              0,
                              0,
                              0,
                              NULL,
                          });
}
int ir_add_fun_call(IR_Bytecode_Array_ptr ir, int args) {
  return ir_add_instr(ir, (IR_Instruct_t){
                              OP_FUN_CALL,
                              0,
                              0,
                              args,
                              NULL,
                          });
}

int ir_add_jump(IR_Bytecode_Array_ptr ir, void *block_ptr) {
  return ir_add_instr(ir, (IR_Instruct_t){
                              OP_JUMP,
                              0,
                              0,
                              0,
                              block_ptr,
                          });
}

int ir_add_load_imm(IR_Bytecode_Array_ptr ir, int reg, int imm) {
  return ir_add_instr(ir, (IR_Instruct_t){
                              OP_LOAD_IMM,
                              BC_REG_PACK(reg, 0),
                              0,
                              imm,
                              NULL,
                          });
}
int ir_add_load_string(IR_Bytecode_Array_ptr ir, int reg, int string) {
  return ir_add_instr(ir, (IR_Instruct_t){
                              OP_LOAD_STRING,
                              BC_REG_PACK(reg, 0),
                              0,
                              string,
                              NULL,
                          });
}
int ir_add_load_prop(IR_Bytecode_Array_ptr ir, int reg, int prop_key) {
  return ir_add_instr(ir, (IR_Instruct_t){
                              OP_LOAD_PROP,
                              BC_REG_PACK(reg, 0),
                              0,
                              prop_key,
                              NULL,
                          });
}
int ir_add_halt(IR_Bytecode_Array_ptr ir) {
  return ir_add_instr(ir, (IR_Instruct_t){
                              OP_HALT,
                              0,
                              0,
                              0,
                              NULL,
                          });
}
