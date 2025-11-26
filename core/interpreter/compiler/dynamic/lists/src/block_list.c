#include "block_list.h"
#include "gc_header.h"
#include "logs.h"
#include "vm_consts.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_igc.h"

#define error(msg, ...) log_add(NULL, ERROR, "Block list", msg, ##__VA_ARGS__)

block_node_ptr block_new(void) {
  block_node_ptr new_block = Xen_Alloc(sizeof(block_node_t));
  if (!new_block) {
    error("Memoria insuficiente");
    return NULL;
  }
  new_block->instr_array = ir_new();
  if (!new_block->instr_array) {
    Xen_Dealloc(new_block);
    return NULL;
  }
  new_block->next = NULL;
  new_block->ready = 0;
  return new_block;
}

void block_free(block_node_ptr block) {
  if (!block) {
    error("Bloque nulo");
    return;
  }
  ir_free(block->instr_array);
  Xen_Dealloc(block);
}

block_list_ptr block_list_new(void) {
  block_list_ptr new_list = Xen_Alloc(sizeof(block_list_t));
  if (!new_list) {
    error("Memoria insuficiente");
    return NULL;
  }
  new_list->consts = vm_consts_new();
  if (!new_list->consts) {
    Xen_Dealloc(new_list);
    return NULL;
  }
  Xen_GC_Push_Root((Xen_GCHeader*)new_list->consts);
  new_list->head = NULL;
  new_list->tail = NULL;
  return new_list;
}

int block_list_push_node(block_list_ptr blocks, block_node_ptr block) {
  if (!blocks || !block) {
    error("Bloque nulo");
    return 0;
  }
  if (!blocks->head)
    blocks->head = block;
  else
    blocks->tail->next = block;
  blocks->tail = block;
  return 1;
}

void block_list_free(block_list_ptr blocks) {
  if (!blocks) {
    error("lista de bloques nula");
    return;
  }
  Xen_GC_Pop_Root();
  block_node_ptr current = blocks->head;
  while (current) {
    block_node_ptr next = current->next;
    block_free(current);
    current = next;
  }
  blocks->head = NULL;
  blocks->tail = NULL;
  Xen_Dealloc(blocks);
}
