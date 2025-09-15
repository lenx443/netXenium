#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "ast_compiler.h"
#include "block_list.h"
#include "ir_bytecode.h"
#include "logs.h"
#include "vm_consts.h"
#include "xen_vector.h"

#define error(msg, ...) log_add(NULL, ERROR, "AST Compiler", msg, ##__VA_ARGS__)
#define info(msg, ...) log_add(NULL, INFO, "AST Compiler", msg, ##__VA_ARGS__)

static int compile_cmd(block_list_ptr, block_node_ptr, AST_Node_t *);

static int store_arg_literal(block_list_ptr, block_node_ptr, ArgExpr_t *, int);
static int store_arg_property(block_list_ptr, block_node_ptr, ArgExpr_t *, int);

int ast_compile(block_list_ptr block_result, block_node_ptr *block, AST_Node_t **ast,
                size_t ast_count) {
  if (!ast) {
    error("arreglo de arboles ast nulo");
    return 1;
  }
  for (int ast_iterator = 0; ast_iterator < ast_count; ast_iterator++) {
    switch (ast[ast_iterator]->ast_type) {
    case AST_EMPTY: break;
    case AST_CMD: compile_cmd(block_result, *block, ast[ast_iterator]); break;
    }
  }
  return 1;
}

int compile_cmd(block_list_ptr blocks, block_node_ptr block, AST_Node_t *ast) {
  if (!block || !ast) {
    error("No se pudo compilar el commndo");
    return 0;
  }
  int cmd_name_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, ast->cmd.cmd_name)) return 0;
  if (!ir_add_load_string(block->instr_array, 0, cmd_name_index)) return 0;
  for (int arg_iterator = 0; arg_iterator < ast->cmd.arg_count; arg_iterator++) {
    ArgExpr_t *arg = ast->cmd.cmd_args[arg_iterator];
    switch (arg->arg_type) {
    case ARG_LITERAL:
      if (!store_arg_literal(blocks, block, arg, arg_iterator + 1)) return 0;
      break;
    case ARG_PROPERTY:
      if (!store_arg_property(blocks, block, arg, arg_iterator + 1)) return 0;
      break;
    }
  }
  if (!ir_add_fun_call(block->instr_array, ast->cmd.arg_count)) return 0;
  return 1;
}

int store_arg_literal(block_list_ptr blocks, block_node_ptr block, ArgExpr_t *arg,
                      int reg) {
  int literal_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, arg->literal)) return 0;
  if (!ir_add_load_string(block->instr_array, reg, literal_index)) return 0;
  return 1;
}

int store_arg_property(block_list_ptr blocks, block_node_ptr block, ArgExpr_t *arg,
                       int reg) {
  int prop_name_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, arg->property)) return 0;
  if (!ir_add_load_prop(block->instr_array, reg, prop_name_index)) return 0;
  return 1;
}
