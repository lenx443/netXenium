#include "ast_compiler.h"
#include "block_list.h"
#include "blocks_compiler.h"
#include "blocks_linealizer.h"
#include "bytecode.h"
#include "callable.h"
#include "instance.h"
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
#include "xen_ast.h"
#include "xen_nil.h"

#define error(msg, ...) log_add(NULL, ERROR, program.name, msg, ##__VA_ARGS__)
#define info(msg, ...) log_add(NULL, INFO, program.name, msg, ##__VA_ARGS__)

int interpreter(const char *text_code) {
  if (!text_code) {
    error("Codigo invalido");
    return 0;
  }
  Lexer lexer = {text_code, 0};
  Parser parser = {&lexer, {0, "\0"}};
  parser_next(&parser);
  Xen_Instance *ast_program = Xen_AST_Node_New("program", NULL);
  if_nil_eval(ast_program) return 0;
  Xen_Instance *current_ast = nil;
  while ((current_ast = parser_stmt(&parser)) != nil) {
    if (!Xen_AST_Node_Push_Child(ast_program, current_ast)) {
      Xen_DEL_REF(ast_program);
      Xen_DEL_REF(current_ast);
      return 0;
    }
    Xen_DEL_REF(current_ast);
  }
  log_show_and_clear(NULL);
  block_list_ptr blocks = block_list_new();
  if (!blocks) { Xen_DEL_REF(ast_program); }
  block_node_ptr main_block = block_new();
  if (!main_block) {
    Xen_DEL_REF(ast_program);
    block_list_free(blocks);
    return 0;
  }
  block_list_push_node(blocks, main_block);
  if (!ast_compile(blocks, &main_block, ast_program)) {
    Xen_DEL_REF(ast_program);
    block_list_free(blocks);
    return 0;
  }
  Xen_DEL_REF(ast_program);
  ir_add_halt(main_block->instr_array);
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
