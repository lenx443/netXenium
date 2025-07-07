#include <malloc.h>
#include <stdlib.h>

#include "block_list.h"
#include "logs.h"
#include "vm_string_table.h"

#define error(msg, ...) log_add(NULL, ERROR, "Block list", msg, ##__VA_ARGS__)

block_node_ptr block_new() {
  block_node_ptr new_block = malloc(sizeof(block_node_t));
  if (!new_block) {
    error("Memoria insuficiente");
    return NULL;
  }
  new_block->instr_array = ir_new();
  if (!new_block->instr_array) {
    free(new_block);
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
  free(block);
}

block_list_ptr block_list_new() {
  block_list_ptr new_list = malloc(sizeof(block_list_t));
  if (!new_list) {
    error("Memoria insuficiente");
    return NULL;
  }
  new_list->strings = vm_string_table_new();
  if (!new_list->strings) {
    free(new_list);
    return NULL;
  }
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
  vm_string_table_free(blocks->strings);
  block_node_ptr current = blocks->head;
  while (!current) {
    block_node_ptr next = current->next;
    block_free(current);
    current = next;
  }
  blocks->head = NULL;
  blocks->tail = NULL;
}
