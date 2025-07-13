#ifndef __BC_INSTRUCT_H__
#define __BC_INSTRUCT_H__

#include <stdint.h>

typedef enum {
  OP_NOP = 0,
  OP_SYSCALL,
  OP_FUN_CALL,
  OP_JUMP_IF_SQUAD,
  OP_LOAD_IMM,
  OP_LOAD_STRING,
  OP_LOAD_PROP,
  OP_STRING_CONCAT,
  OP_REG_CONCAT,
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

typedef union bc_Instruct bc_Instruct_t;
typedef bc_Instruct_t *bc_Instruct_ptr;

#endif
