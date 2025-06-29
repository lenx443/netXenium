#ifndef __BYTECODE_H__
#define __BYTECODE_H__

#include <stddef.h>
#include <stdint.h>

typedef enum {
  OP_NOP = 0,
  OP_LOAD_IMM,
  OP_LOAD_STRING,
  OP_HALT,
} bc_opcode;

#pragma pack(push, 1)
union bc_Instruct {
  struct {
    uint8_t bci_opcode;
    uint8_t bci_dst;
    uint8_t bci_src1;
    uint8_t bci_src2;
  };
  uint32_t bci_word;
};
#pragma pack(pop)

struct Bytecode_Array {
  union bc_Instruct *bc_array;
  size_t bc_size;
  size_t bc_capacity;
};

#define BC_REG_PACK(reg, flag) ((flag ? 0x80 : 0) | (reg & 0x7f))
#define BC_REG_GET_FLAG_BIT(reg) (reg & 0x80) >> 7
#define BC_REG_GET_VALUE(reg) (reg & 0x7f)

typedef union bc_Instruct bc_Instruct_t;
typedef struct Bytecode_Array Bytecode_Array_t;
typedef bc_Instruct_t *bc_Instruct_ptr;
typedef Bytecode_Array_t *Bytecode_Array_ptr;

Bytecode_Array_ptr bc_new();
void bc_free(const Bytecode_Array_ptr);
int bc_add_instr(Bytecode_Array_ptr, bc_Instruct_t);
int bc_add_nop(Bytecode_Array_ptr);
int bc_add_load_imm(Bytecode_Array_ptr, int, int);
int bc_add_load_string(Bytecode_Array_ptr, int, int);
int bc_add_halt(Bytecode_Array_ptr);

#endif
