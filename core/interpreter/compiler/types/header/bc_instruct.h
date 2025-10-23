#ifndef __BC_INSTRUCT_H__
#define __BC_INSTRUCT_H__

#include <stdint.h>

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
typedef bc_Instruct_t* bc_Instruct_ptr;

#endif
