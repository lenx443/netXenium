#ifndef __BYTECODE_H__
#define __BYTECODE_H__

#include <stddef.h>
#include <stdint.h>

#include "bc_instruct.h"

struct Bytecode_Array {
  union bc_Instruct *bc_array;
  size_t bc_size;
  size_t bc_capacity;
};

#define BC_REG_PACK(reg, flag) ((flag ? 0x80 : 0) | (reg & 0x7f))
#define BC_REG_GET_FLAG_BIT(reg) (reg & 0x80) >> 7
#define BC_REG_GET_VALUE(reg) (reg & 0x7f)

typedef struct Bytecode_Array Bytecode_Array_t;
typedef Bytecode_Array_t *Bytecode_Array_ptr;

Bytecode_Array_ptr bc_new();
void bc_clear(Bytecode_Array_ptr);
void bc_free(const Bytecode_Array_ptr);
int bc_add_instr(Bytecode_Array_ptr, bc_Instruct_t);
int bc_add_nop(Bytecode_Array_ptr);
int bc_add_syscall(Bytecode_Array_ptr);
int bc_add_fun_call(Bytecode_Array_ptr, int);
int bc_add_load_imm(Bytecode_Array_ptr, int, int);
int bc_add_load_string(Bytecode_Array_ptr, int, int);
int bc_add_load_prop(Bytecode_Array_ptr, int, int);
int bc_add_string_concat(Bytecode_Array_ptr, int, int, int);
int bc_add_reg_concat(Bytecode_Array_ptr, int, int, int);
int bc_add_halt(Bytecode_Array_ptr);

#endif
