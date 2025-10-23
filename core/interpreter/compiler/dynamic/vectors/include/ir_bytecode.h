#ifndef __IR_BYTECODE_H__
#define __IR_BYTECODE_H__

#include <stddef.h>

#include "ir_instruct.h"

struct IR_Bytecode_Array {
  struct IR_Instruct* ir_array;
  size_t ir_size;
  size_t ir_capacity;
};

typedef struct IR_Bytecode_Array IR_Bytecode_Array_t;
typedef IR_Bytecode_Array_t* IR_Bytecode_Array_ptr;

IR_Bytecode_Array_ptr ir_new();
void ir_free(const IR_Bytecode_Array_ptr);
int ir_add_instr(IR_Bytecode_Array_ptr, IR_Instruct_t);

#endif
