#include "block_list.h"
#include "blocks_linealizer.h"

void blocks_linealizer(block_list_ptr blocks) {
  if (!blocks) return;
  block_node_ptr current_block = blocks->head;
  int n = 0;
  while (current_block) {
    for (int i = 0; i < current_block->instr_array->ir_size; i++)
      current_block->instr_array->ir_array[i].instr_num = n++;
    current_block->ready = 1;
    current_block = current_block->next;
  }
}
