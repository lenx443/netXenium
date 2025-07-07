#include <stdio.h>

#include "ast.h"
#include "ast_compiler.h"
#include "block_list.h"
#include "logs.h"
#include "vm_string_table.h"

#define error(msg, ...) log_add(NULL, ERROR, "AST Compiler", msg, ##__VA_ARGS__)

static int compile_cmd(block_list_ptr, block_node_ptr, AST_Node_t *);

static int store_arg_literal(block_list_ptr, block_node_ptr, ArgExpr_t *, int);
static int store_arg_property(block_list_ptr, block_node_ptr, ArgExpr_t *, int);
static int store_arg_concat(block_list_ptr, block_node_ptr, ArgExpr_t *, int);

block_list_ptr ast_compile(AST_Node_t **ast, size_t ast_count) {
  if (!ast) {
    error("arreglo de arboles ast nulo");
    return NULL;
  }
  block_list_ptr block_result = block_list_new();
  if (!block_result) return NULL;
  block_node_ptr block_main = block_new();
  if (!block_main) {
    block_list_free(block_result);
    return NULL;
  }
  for (int ast_iterator = 0; ast_iterator < ast_count; ast_iterator++) {
    switch (ast[ast_iterator]->ast_type) {
    case AST_EMPTY: ir_add_nop(block_main->instr_array); break;
    case AST_CMD: compile_cmd(block_result, block_main, ast[ast_iterator]);
    }
  }
  ir_add_halt(block_main->instr_array);
  block_list_push_node(block_result, block_main);
  return block_result;
}

int compile_cmd(block_list_ptr blocks, block_node_ptr block, AST_Node_t *ast) {
  if (!block || !ast) {
    error("No se pudo compilar el commndo");
    return 0;
  }
  int cmd_name_index = blocks->strings->size;
  if (!vm_string_table_add(blocks->strings, ast->cmd.cmd_name)) return 0;
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
    case ARG_CONCAT:
      if (!store_arg_concat(blocks, block, arg, arg_iterator + 1)) return 0;
      break;
    }
  }
  if (!ir_add_fun_call(block->instr_array, ast->cmd.arg_count)) return 0;
  return 1;
}

int store_arg_literal(block_list_ptr blocks, block_node_ptr block, ArgExpr_t *arg,
                      int reg) {
  int literal_index = blocks->strings->size;
  if (!vm_string_table_add(blocks->strings, arg->literal)) return 0;
  if (!ir_add_load_string(block->instr_array, reg, literal_index)) return 0;
  return 1;
}

int store_arg_property(block_list_ptr blocks, block_node_ptr block, ArgExpr_t *arg,
                       int reg) {
  int prop_name_index = blocks->strings->size;
  if (!vm_string_table_add(blocks->strings, arg->property)) return 0;
  if (!ir_add_load_prop(block->instr_array, reg, prop_name_index)) return 0;
  return 1;
}

int store_arg_concat(block_list_ptr blocks, block_node_ptr block, ArgExpr_t *arg,
                     int reg) {
  if (arg->concat.count <= 0) return 1;
  switch (arg->concat.parts[0]->arg_type) {
  case ARG_LITERAL:
    if (!store_arg_literal(blocks, block, arg->concat.parts[0], reg)) return 0;
    break;
  case ARG_PROPERTY:
    if (!store_arg_property(blocks, block, arg->concat.parts[0], reg)) return 0;
    break;
  default: return 0;
  }
  for (int i = 1; i < arg->concat.count; i++) {
    ArgExpr_t *current_arg = arg->concat.parts[i];
    switch (current_arg->arg_type) {
    case ARG_LITERAL:
      if (!store_arg_literal(blocks, block, current_arg, reg + 1)) return 0;
      break;
    case ARG_PROPERTY:
      if (!store_arg_property(blocks, block, current_arg, reg + 1)) return 0;
      break;
    default: return 0;
    }
    if (!ir_add_reg_concat(block->instr_array, reg, reg, reg + 1)) return 0;
  }
  return 1;
}
