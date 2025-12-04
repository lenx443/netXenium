#ifndef __IR_ISNTRUCT_H__
#define __IR_ISNTRUCT_H__

#include "source_file.h"
#include "xen_typedefs.h"

struct block_node;

struct IR_Instruct {
  Xen_uint8_t opcode;
  Xen_ulong_t oparg;
  Xen_uint8_t is_jump;
  struct block_node* jump_block;
  Xen_ulong_t instr_num;
  Xen_Source_Address sta;
};

typedef struct IR_Instruct IR_Instruct_t;
typedef IR_Instruct_t* IR_Instruct_ptr;

#endif
