#ifndef __BYTECODE_H__
#define __BYTECODE_H__

#include <stddef.h>
#include <stdint.h>

typedef enum __attribute__((packed)) {
  NOP = 0,
} bc_opcode;

struct bc_Instruct {
  bc_opcode bci_opcode;
  uint8_t bci_dst;
  uint8_t bci_src1;
  uint8_t bci_src2;
};

struct Bytecode_Array {
  struct bc_Instruct *bc_array;
  size_t bc_size;
  size_t bc_capacity;
};

typedef struct bc_Instruct bc_Instruct_t;
typedef struct Bytecode_Array Bytecode_Array_t;
typedef bc_Instruct_t *bc_Instruct_ptr;
typedef Bytecode_Array_t *Bytecode_Array_ptr;

Bytecode_Array_ptr bc_new();
void bc_free(const Bytecode_Array_ptr);
int bc_add_instr(Bytecode_Array_ptr, bc_Instruct_t);
int bc_add_nop(Bytecode_Array_ptr);

#endif
