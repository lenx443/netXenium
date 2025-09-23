#include "ast.h"
#include "ast_array.h"
#include "ast_compiler.h"
#include "block_list.h"
#include "blocks_compiler.h"
#include "blocks_linealizer.h"
#include "bytecode.h"
#include "callable.h"
#include "interpreter.h"
#include "ir_bytecode.h"
#include "lexer.h"
#include "logs.h"
#include "parser.h"
#include "program.h"
#include "program_code.h"
#include "run_ctx.h"
#include "vm.h"
#include "vm_consts.h"
#include "vm_def.h"
#include "vm_run.h"

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
  AST_Node_t *current_ast = NULL;
  while (parser_stmt(&parser, &current_ast) == 1) {
    if (!ast_array_add(ast_array, current_ast)) {
      ast_array_free(ast_array);
      ast_free(current_ast);
      return 0;
    }
    current_ast = NULL;
  }
  log_show_and_clear(NULL);
  if (current_ast) {
    ast_free(current_ast);
    current_ast = NULL;
  }
  if (ast_array->ast_array == NULL) {
    ast_array_free(ast_array);
    return 1;
  }
  block_list_ptr blocks = block_list_new();
  if (!blocks) { ast_array_free(ast_array); }
  block_node_ptr main_block = block_new();
  if (!main_block) {
    ast_array_free(ast_array);
    block_list_free(blocks);
    return 0;
  }
  block_list_push_node(blocks, main_block);
  AST_Node_t *ast_program =
      ast_node_make("program", NULL, ast_array->ast_count, ast_array->ast_array);
  if (!ast_compile(blocks, &main_block, ast_program)) {
    ast_array_free(ast_array);
    block_list_free(blocks);
    return 0;
  }
  free(ast_program);
  ir_add_halt(main_block->instr_array);
  ast_array_free(ast_array);
  blocks_linealizer(blocks);
  ProgramCode_t pc;
  if (!blocks_compiler(&pc, blocks)) {
    block_list_free(blocks);
    return 0;
  }
  block_list_free(blocks);
  vm_ctx_clear(vm->root_context);
  CALLABLE_ptr code = callable_new_code(pc);
  if (!code) {
    vm_consts_free(pc.consts);
    bc_free(pc.code);
    return 0;
  }
  vm->root_context->ctx_code = code;
  vm_run_ctx(vm->root_context);
  callable_free(code);
  log_show_and_clear(NULL);
  return 1;
}
