#include "interpreter.h"
#include "ast_compiler.h"
#include "block_list.h"
#include "blocks_compiler.h"
#include "blocks_linealizer.h"
#include "bytecode.h"
#include "callable.h"
#include "instance.h"
#include "ir_bytecode.h"
#include "lexer.h"
#include "logs.h"
#include "parser.h"
#include "program.h"
#include "program_code.h"
#include "vm.h"
#include "vm_consts.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_ast.h"
#include "xen_nil.h"

#define error(msg, ...) log_add(NULL, ERROR, program.name, msg, ##__VA_ARGS__)
#define info(msg, ...) log_add(NULL, INFO, program.name, msg, ##__VA_ARGS__)

int interpreter(const char* text_code) {
  if (!text_code) {
    error("Codigo invalido");
    return 0;
  }
  Lexer lexer = {text_code, 0};
  Parser parser = {&lexer, {0, "\0"}};
  parser_next(&parser);
  Xen_Instance* ast_program = parser_program(&parser);
  if_nil_eval(ast_program) return 0;
  Xen_AST_Node_Print(ast_program);
  Xen_DEL_REF(ast_program);
  return 1;
  block_list_ptr blocks = block_list_new();
  if (!blocks) {
    Xen_DEL_REF(ast_program);
  }
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
  if (vm_run_ctx(vm->root_context) == NULL) {
    callable_free(code);
    return 0;
  }
  callable_free(code);
  log_show_and_clear(NULL);
  return 1;
}
