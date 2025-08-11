#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "ast_compiler.h"
#include "block_list.h"
#include "ir_bytecode.h"
#include "logs.h"
#include "vm_consts.h"

#define error(msg, ...) log_add(NULL, ERROR, "AST Compiler", msg, ##__VA_ARGS__)
#define info(msg, ...) log_add(NULL, INFO, "AST Compiler", msg, ##__VA_ARGS__)

static int compile_if(block_list_ptr, block_node_ptr *, AST_Node_t *);
static int compile_cmd(block_list_ptr, block_node_ptr, AST_Node_t *);

static int store_arg_literal(block_list_ptr, block_node_ptr, ArgExpr_t *, int);
static int store_arg_property(block_list_ptr, block_node_ptr, ArgExpr_t *, int);
static int store_arg_concat(block_list_ptr, block_node_ptr, ArgExpr_t *, int);

int ast_compile(block_list_ptr block_result, block_node_ptr *block, AST_Node_t **ast,
                size_t ast_count) {
  if (!ast) {
    error("arreglo de arboles ast nulo");
    return 1;
  }
  for (int ast_iterator = 0; ast_iterator < ast_count; ast_iterator++) {
    switch (ast[ast_iterator]->ast_type) {
    case AST_EMPTY: break;
    case AST_IF: compile_if(block_result, block, ast[ast_iterator]); break;
    case AST_CMD: compile_cmd(block_result, *block, ast[ast_iterator]); break;
    }
  }
  return 1;
}

static int compile_if(block_list_ptr blocks, block_node_ptr *block, AST_Node_t *ast) {
  if (!*block || !ast) {
    error("No se pudo compilar la condional");
    return 0;
  }
  int b1_n = blocks->consts->c_names_size;
  vm_consts_push_name(blocks->consts, ast->if_conditional.condition->pair.c1->content);
  if (ast->if_conditional.condition->pair.c1->type == BOOL_LITERAL) {
    ir_add_load_string((*block)->instr_array, 1, b1_n);
  } else if (ast->if_conditional.condition->pair.c1->type == BOOL_PROPERTY) {
    ir_add_load_prop((*block)->instr_array, 1, b1_n);
  }
  int b2_n = blocks->consts->c_names_size;
  vm_consts_push_name(blocks->consts, ast->if_conditional.condition->pair.c2->content);
  if (ast->if_conditional.condition->pair.c2->type == BOOL_LITERAL) {
    ir_add_load_string((*block)->instr_array, 2, b2_n);
  } else if (ast->if_conditional.condition->pair.c2->type == BOOL_PROPERTY) {
    ir_add_load_prop((*block)->instr_array, 2, b2_n);
  }
  block_node_ptr current_block = block_new();
  block_node_ptr new_block = current_block;
  if (!new_block) { return 0; }
  block_list_push_node(blocks, new_block);
  if (!ast_compile(blocks, &current_block, ast->if_conditional.body,
                   ast->if_conditional.body_count)) {
    return 0;
  }
  log_show_and_clear(NULL);
  ir_add_jump_if_squad((*block)->instr_array, new_block);
  block_node_ptr end_block = block_new();
  if (!end_block) { return 0; }
  block_list_push_node(blocks, end_block);
  ir_add_jump((*block)->instr_array, end_block);
  ir_add_jump(current_block->instr_array, end_block);
  log_show_and_clear(NULL);
  *block = end_block;
  return 1;
}

int compile_cmd(block_list_ptr blocks, block_node_ptr block, AST_Node_t *ast) {
  if (!block || !ast) {
    error("No se pudo compilar el commndo");
    return 0;
  }
  int cmd_name_index = blocks->consts->c_names_size;
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
  int literal_index = blocks->consts->c_names_size;
  if (!vm_consts_push_name(blocks->consts, arg->literal)) return 0;
  if (!ir_add_load_string(block->instr_array, reg, literal_index)) return 0;
  return 1;
}

int store_arg_property(block_list_ptr blocks, block_node_ptr block, ArgExpr_t *arg,
                       int reg) {
  int prop_name_index = blocks->consts->c_names_size;
  if (!vm_consts_push_name(blocks->consts, arg->property)) return 0;
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
