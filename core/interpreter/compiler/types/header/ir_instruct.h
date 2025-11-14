#ifndef __IR_ISNTRUCT_H__
#define __IR_ISNTRUCT_H__

#include <stdint.h>

struct block_node;

struct IR_Instruct {
  uint8_t opcode;
  uint8_t oparg;
  uint8_t is_jump;
  struct block_node* jump_block;
  int instr_num;
};

typedef struct IR_Instruct IR_Instruct_t;
typedef IR_Instruct_t* IR_Instruct_ptr;

#endif
