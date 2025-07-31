#ifndef __IR_ISNTRUCT_H__
#define __IR_ISNTRUCT_H__

#include <stdint.h>

struct IR_Instruct {
  uint8_t opcode;
  uint8_t dst;
  uint8_t src1;
  uint8_t src2;
  void *jump_block;
  int instr_num;
};

typedef struct IR_Instruct IR_Instruct_t;
typedef IR_Instruct_t *IR_Instruct_ptr;

#endif
