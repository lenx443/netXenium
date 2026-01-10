#include "block_list.h"
#include "gc_header.h"
#include "vm_consts.h"
#include "xen_alloc.h"
#include "xen_gc.h"

block_node_ptr block_new(void) {
  block_node_ptr new_block = Xen_Alloc(sizeof(block_node_t));
  if (!new_block) {
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
    return;
  }
  ir_free(block->instr_array);
  Xen_Dealloc(block);
}

static void block_list_trace(Xen_GCHeader* h) {
  block_list_ptr blocks = (block_list_ptr)h;
  Xen_GC_Trace_GCHeader(blocks->consts);
}

static void block_list_free(Xen_GCHeader* h) {
  block_list_ptr blocks = (block_list_ptr)h;
  if (!blocks) {
    return;
  }
  Xen_GCHandle_Free(blocks->consts);
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

block_list_ptr block_list_new(void) {
  block_list_ptr new_list = (block_list_ptr)Xen_GC_New(
      sizeof(block_list_t), block_list_trace, block_list_free);
  if (!new_list) {
    return NULL;
  }
  new_list->consts = Xen_GCHandle_New_From((struct __GC_Header*)new_list,
                                           (Xen_GCHeader*)vm_consts_new());
  if (!new_list->consts) {
    Xen_Dealloc(new_list);
    return NULL;
  }
  new_list->head = NULL;
  new_list->tail = NULL;
  return new_list;
}

int block_list_push_node(block_list_ptr blocks, block_node_ptr block) {
  if (!blocks || !block) {
    return 0;
  }
  if (!blocks->head)
    blocks->head = block;
  else
    blocks->tail->next = block;
  blocks->tail = block;
  return 1;
}
