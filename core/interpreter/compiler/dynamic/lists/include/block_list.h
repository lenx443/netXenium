#ifndef __BLOCK_LIST_H__
#define __BLOCK_LIST_H__

#include "ir_bytecode.h"
#include "vm_consts.h"

struct block_node {
  IR_Bytecode_Array_ptr instr_array;
  struct block_node *next;
  int ready;
};

struct block_list {
  vm_Consts_ptr consts;
  struct block_node *head;
  struct block_node *tail;
};

typedef struct block_node block_node_t;
typedef block_node_t *block_node_ptr;
typedef struct block_list block_list_t;
typedef block_list_t *block_list_ptr;

block_node_ptr block_new();
void block_free(block_node_ptr);

block_list_ptr block_list_new();
int block_list_push_node(block_list_ptr, block_node_ptr);
void block_list_free(block_list_ptr);

#endif
