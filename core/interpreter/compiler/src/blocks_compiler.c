#include "bc_instruct.h"
#include "block_list.h"
#include "blocks_compiler.h"
#include "bytecode.h"
#include "logs.h"
#include "vm_consts.h"

#define error(msg, ...) log_add(NULL, ERROR, "Blocks Compiler", msg, ##__VA_ARGS__)

int blocks_compiler(ProgramCode_t *code, block_list_ptr blocks) {
  if (!blocks) {
    error("lista de bloques nula");
    return 0;
  }
  code->consts = vm_consts_new();
  if (!code->consts) {
    error("Memoria insufciente");
    return 0;
  }
  code->code = bc_new();
  if (!code->code) {
    error("Memoria insufciente");
    vm_consts_free(code->consts);
    return 0;
  }
  for (int name_iterator = 0; name_iterator < blocks->consts->c_names_size;
       name_iterator++) {
    if (!vm_consts_push_name(code->consts, blocks->consts->c_names[name_iterator])) {
      error("Memoria insufciente");
      vm_consts_free(code->consts);
      bc_free(code->code);
      return 0;
    }
  }
  for (int instance_iterator = 0; instance_iterator < blocks->consts->c_instances_size;
       instance_iterator++) {
    if (!vm_consts_push_instance(code->consts,
                                 blocks->consts->c_instances[instance_iterator], true))
      ;
  }
  block_node_ptr current_block = blocks->head;
  while (current_block) {
    if (!current_block->ready) {
      current_block = current_block->next;
      continue;
    }
    for (int instr_iterator = 0; instr_iterator < current_block->instr_array->ir_size;
         instr_iterator++) {
      if (current_block->instr_array->ir_array[instr_iterator].opcode ==
              OP_JUMP_IF_SQUAD ||
          current_block->instr_array->ir_array[instr_iterator].opcode == OP_JUMP) {

        if (!bc_add_instr(
                code->code,
                (bc_Instruct_t){
                    current_block->instr_array->ir_array[instr_iterator].opcode,
                    current_block->instr_array->ir_array[instr_iterator].dst,
                    current_block->instr_array->ir_array[instr_iterator].src1,
                    ((block_node_ptr)current_block->instr_array->ir_array[instr_iterator]
                         .jump_block)
                        ->instr_array->ir_array[0]
                        .instr_num,
                })) {
          error("Memoria insufciente");
          vm_consts_free(code->consts);
          bc_free(code->code);
          return 0;
        }
      } else {
        if (!bc_add_instr(code->code,
                          (bc_Instruct_t){
                              current_block->instr_array->ir_array[instr_iterator].opcode,
                              current_block->instr_array->ir_array[instr_iterator].dst,
                              current_block->instr_array->ir_array[instr_iterator].src1,
                              current_block->instr_array->ir_array[instr_iterator].src2,
                          })) {
          error("Memoria insufciente");
          vm_consts_free(code->consts);
          bc_free(code->code);
          return 0;
        }
      }
    }
    current_block = current_block->next;
  }
  return 1;
}
