#include "block_list.h"
#include "gc_header.h"
#include "program_code.h"
#include "vm_instructs.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_typedefs.h"

#ifndef EXT_ARG_OP
#define EXT_ARG_OP 0xFF
#endif

typedef struct {
  Xen_size_t pc_offset;
  IR_Instruct_t* inst;
} Reloc;

static int reloc_add(Reloc** table, Xen_size_t* count, Xen_size_t* cap,
                     IR_Instruct_t* inst, Xen_size_t pos) {
  if (*count == *cap) {
    Xen_size_t newcap = (*cap == 0) ? 32 : (*cap * 2);
    Reloc* tmp = Xen_Realloc(*table, newcap * sizeof(Reloc));
    if (!tmp)
      return 0;
    *table = tmp;
    *cap = newcap;
  }
  (*table)[*count].inst = inst;
  (*table)[*count].pc_offset = pos;
  (*count)++;
  return 1;
}

int blocks_linealizer(block_list_ptr blocks) {
  if (!blocks)
    return 0;
  block_node_ptr current_block = blocks->head;
  Xen_ulong_t n = 0;
  while (current_block) {
    if (current_block->instr_array->ir_size == 0) {
      if (!ir_emit(current_block->instr_array, NOP, 0)) {
        return 0;
      }
    }
    for (Xen_size_t i = 0; i < current_block->instr_array->ir_size; i++) {
      current_block->instr_array->ir_array[i].instr_num = n++;
    }
    current_block->ready = 1;
    current_block = current_block->next;
  }
  return 1;
}

int blocks_compiler(block_list_ptr blocks, ProgramCode_t* pc) {
  if (!blocks || !pc)
    return 0;

  pc->code = bc_new();
  if (!pc->code)
    return 0;

  pc->consts = vm_consts_from_values(blocks->consts->c_names,
                                     blocks->consts->c_instances);
  if (!pc->consts) {
    bc_free(pc->code);
    return 0;
  }
  Xen_GC_Push_Root((Xen_GCHeader*)pc->consts);

  Xen_size_t total_ir_count = 0;
  block_node_ptr btmp = blocks->head;
  while (btmp) {
    for (Xen_size_t i = 0; i < btmp->instr_array->ir_size; i++) {
      Xen_ulong_t num = btmp->instr_array->ir_array[i].instr_num;
      if ((Xen_size_t)num + 1 > total_ir_count)
        total_ir_count = (Xen_size_t)num + 1;
    }
    btmp = btmp->next;
  }
  if (total_ir_count == 0)
    total_ir_count = 1;

  Xen_size_t* real_offset = Xen_ZAlloc(total_ir_count, sizeof(Xen_size_t));
  if (!real_offset)
    goto error_alloc;

  Reloc* reloc_table = NULL;
  Xen_size_t reloc_count = 0;
  Xen_size_t reloc_cap = 0;

  Xen_long_t depth = 0;
  Xen_long_t effect = 0;

  block_node_ptr b_iter = blocks->head;
  while (b_iter) {
    if (!b_iter->ready) {
      b_iter = b_iter->next;
      continue;
    }

    for (Xen_size_t i_iter = 0; i_iter < b_iter->instr_array->ir_size;
         i_iter++) {
      IR_Instruct_t* inst = &b_iter->instr_array->ir_array[i_iter];

      real_offset[inst->instr_num] = pc->code->bc_size;

      if (inst->is_jump) {
        Xen_ulong_t target_ir =
            inst->jump_block->instr_array->ir_array[0].instr_num;

        if (target_ir >= 0xFF) {
          if (!bc_emit(pc->code, inst->opcode, 0xFF))
            goto error;
          Xen_size_t argpos = pc->code->bc_size;
          for (Xen_size_t j = 0; j < XEN_ULONG_SIZE; j++) {
            if (!bc_emit(pc->code, EXT_ARG_OP, 0x00))
              goto error;
          }
          if (!reloc_add(&reloc_table, &reloc_count, &reloc_cap, inst, argpos))
            goto error;
        } else {
          if (!bc_emit(pc->code, inst->opcode, (uint8_t)target_ir))
            goto error;
        }
      } else {
        if (inst->oparg >= 0xFF) {
          if (!bc_emit(pc->code, inst->opcode, 0xFF))
            goto error;
          Xen_size_t argpos = pc->code->bc_size;
          for (Xen_size_t j = 0; j < XEN_ULONG_SIZE; j++) {
            if (!bc_emit(pc->code, EXT_ARG_OP, 0x00))
              goto error;
          }
          if (!reloc_add(&reloc_table, &reloc_count, &reloc_cap, inst, argpos))
            goto error;
        } else {
          if (!bc_emit(pc->code, inst->opcode, (uint8_t)inst->oparg))
            goto error;
        }
      }
      effect += Instruct_Info_Table[inst->opcode].stack_effect(inst->oparg);
      if (effect > depth)
        depth = effect;
    }
    b_iter = b_iter->next;
  }

  for (Xen_size_t r = 0; r < reloc_count; r++) {
    Reloc* rel = &reloc_table[r];
    IR_Instruct_t* inst = rel->inst;
    Xen_size_t pos = rel->pc_offset;

    Xen_ulong_t value;
    if (inst->is_jump) {
      Xen_ulong_t target_ir =
          inst->jump_block->instr_array->ir_array[0].instr_num;
      value = (Xen_ulong_t)real_offset[target_ir];
    } else {
      value = inst->oparg;
    }

    for (Xen_size_t b = 0; b < XEN_ULONG_SIZE; b++) {
      Xen_size_t slot_idx = pos + b;
      if (slot_idx >= pc->code->bc_size)
        goto error;
      bc_Instruct_t* slot = &pc->code->bc_array[slot_idx];
      slot->bci_opcode = EXT_ARG_OP;
      slot->bci_oparg = (uint8_t)((value >> (b * 8)) & 0xFF);
    }
  }

  pc->stack_depth = (Xen_size_t)depth;

  Xen_Dealloc(reloc_table);
  Xen_Dealloc(real_offset);
  Xen_GC_Pop_Root();
  return 1;

error:
  if (reloc_table)
    Xen_Dealloc(reloc_table);
  if (real_offset)
    Xen_Dealloc(real_offset);
  if (pc->code)
    bc_free(pc->code);
  if (pc->consts)
    Xen_GC_Pop_Root();
  return 0;

error_alloc:
  if (pc->code)
    bc_free(pc->code);
  if (pc->consts)
    Xen_GC_Pop_Root();
  return 0;
}
