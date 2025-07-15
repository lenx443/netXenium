#include "ast.h"
#include "ast_array.h"
#include "ast_compiler.h"
#include "bc_instruct.h"
#include "block_list.h"
#include "blocks_compiler.h"
#include "blocks_linealizer.h"
#include "bytecode.h"
#include "interpreter.h"
#include "ir_bytecode.h"
#include "ir_instruct.h"
#include "lexer.h"
#include "logs.h"
#include "parser.h"
#include "program.h"
#include "vm.h"
#include "vm_string_table.h"

#define error(msg, ...) log_add(NULL, ERROR, program.name, msg, ##__VA_ARGS__)
#define info(msg, ...) log_add(NULL, INFO, program.name, msg, ##__VA_ARGS__)

int interpreter(const char *text_code) {
  if (!text_code) {
    error("Codigo invalido");
    return 0;
  }
  Lexer lexer = {text_code, 0};
  Parser parser = {&lexer};
  parser_next(&parser);
  AST_Array_ptr ast_array = ast_array_new();
  if (!ast_array) return 0;
  AST_Node_t *current_ast;
  while (parser_stmt(&parser, &current_ast) == 1) {
    ast_print(current_ast);
    if (!ast_array_add(ast_array, current_ast)) {
      ast_array_free(ast_array);
      return 0;
    }
    current_ast = NULL;
  }
  log_show_and_clear(NULL);
  if (ast_array->ast_array == NULL) {
    ast_array_free(ast_array);
    return 1;
  }
  block_list_ptr blocks = block_list_new();
  if (!blocks) { ast_array_free(ast_array); }
  block_node_ptr main_block = block_new();
  if (!main_block) { ast_array_free(ast_array); }
  block_list_push_node(blocks, main_block);
  if (!ast_compile(blocks, &main_block, ast_array->ast_array, ast_array->ast_count)) {
    ast_array_free(ast_array);
    block_list_free(blocks);
    return 0;
  }
  ir_add_halt(main_block->instr_array);
  ast_array_free(ast_array);
  blocks_linealizer(blocks);
  block_node_ptr current = blocks->head;
  int n = 0;
  while (current) {
    info("BLOCK%d:", n++);
    for (int i = 0; i < current->instr_array->ir_size; i++) {
      IR_Instruct_t instr = current->instr_array->ir_array[i];
      if (instr.opcode == OP_JUMP_IF_SQUAD || instr.opcode == OP_JUMP) {
        info("%d ———— %d, %d, %d, %d", instr.instr_num, instr.opcode,
             BC_REG_GET_VALUE(instr.dst), BC_REG_GET_VALUE(instr.src1),
             ((block_node_ptr)instr.jump_block)->instr_array->ir_array[0].instr_num);
      } else {
        info("%d ———— %d, %d, %d, %d", instr.instr_num, instr.opcode,
             BC_REG_GET_VALUE(instr.dst), BC_REG_GET_VALUE(instr.src1), instr.src2);
      }
    }
    current = current->next;
  }
  log_show_and_clear(NULL);
  ProgramCode_t program_code;
  if (!blocks_compiler(&program_code, blocks)) {
    block_list_free(blocks);
    return 0;
  }
  block_list_free(blocks);
  VM_ptr vm = vm_program_code_new(program_code);
  if (!vm) {
    vm_string_table_free(program_code.strings);
    bc_free(program_code.code);
    return 0;
  }
  log_show_and_clear(NULL);
  vm_run(vm);
  vm_free(vm);
  return 1;
}
