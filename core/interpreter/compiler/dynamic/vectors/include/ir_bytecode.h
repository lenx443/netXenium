#ifndef __IR_BYTECODE_H__
#define __IR_BYTECODE_H__

#include <stddef.h>

#include "ir_instruct.h"

struct IR_Bytecode_Array {
  struct IR_Instruct *ir_array;
  size_t ir_size;
  size_t ir_capacity;
};

#define BC_REG_PACK(reg, flag) ((flag ? 0x80 : 0) | (reg & 0x7f))
#define BC_REG_GET_FLAG_BIT(reg) (reg & 0x80) >> 7
#define BC_REG_GET_VALUE(reg) (reg & 0x7f)

typedef struct IR_Bytecode_Array IR_Bytecode_Array_t;
typedef IR_Bytecode_Array_t *IR_Bytecode_Array_ptr;

IR_Bytecode_Array_ptr ir_new();
void ir_free(const IR_Bytecode_Array_ptr);
int ir_add_instr(IR_Bytecode_Array_ptr, IR_Instruct_t);
int ir_add_nop(IR_Bytecode_Array_ptr);
int ir_add_fun_call(IR_Bytecode_Array_ptr, int);
int ir_add_jump(IR_Bytecode_Array_ptr, void *);
int ir_add_load_imm(IR_Bytecode_Array_ptr, int, int);
int ir_add_load_instance(IR_Bytecode_Array_ptr, int, int);
int ir_add_load_name(IR_Bytecode_Array_ptr, int, int);
int ir_add_load_prop(IR_Bytecode_Array_ptr, int, int);
int ir_add_make_instance(IR_Bytecode_Array_ptr, int, int);
int ir_add_halt(IR_Bytecode_Array_ptr);

#endif
