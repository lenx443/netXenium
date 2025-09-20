#include <stddef.h>
#include <string.h>

#include "ast.h"
#include "ast_compiler.h"
#include "block_list.h"
#include "instance.h"
#include "ir_bytecode.h"
#include "logs.h"
#include "operators.h"
#include "vm_consts.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

#define error(msg, ...) log_add(NULL, ERROR, "AST Compiler", msg, ##__VA_ARGS__)
#define info(msg, ...) log_add(NULL, INFO, "AST Compiler", msg, ##__VA_ARGS__)

static int compile_string(int, block_list_ptr, block_node_ptr, AST_Node_t *);
static int compile_literal(int, block_list_ptr, block_node_ptr, AST_Node_t *);
static int compile_property(int, block_list_ptr, block_node_ptr, AST_Node_t *);
static int compile_assignment(block_list_ptr, block_node_ptr, AST_Node_t *);
static int compile_cmd(block_list_ptr, block_node_ptr, AST_Node_t *);

int ast_compile(block_list_ptr block_result, block_node_ptr *block, AST_Node_t **ast,
                size_t ast_count) {
  if (!ast) {
    error("arreglo de arboles ast nulo");
    return 0;
  }
  for (int ast_iterator = 0; ast_iterator < ast_count; ast_iterator++) {
    switch (ast[ast_iterator]->ast_type) {
    case AST_EMPTY: break;
    case AST_STRING:
      if (!compile_string(1, block_result, *block, ast[ast_iterator])) { return 0; }
      break;
    case AST_LITERAL:
      if (!compile_literal(1, block_result, *block, ast[ast_iterator])) { return 0; }
      break;
    case AST_PROPERTY:
      if (!compile_property(1, block_result, *block, ast[ast_iterator])) { return 0; }
      break;
    case AST_ASSIGNMENT:
      if (!compile_assignment(block_result, *block, ast[ast_iterator])) { return 0; }
      break;
    case AST_CMD:
      if (!compile_cmd(block_result, *block, ast[ast_iterator])) { return 0; }
      break;
    }
  }
  return 1;
}

int compile_string(int out_reg, block_list_ptr blocks, block_node_ptr block,
                   AST_Node_t *ast) {
  Xen_Instance *string = Xen_String_From_CString(ast->string.value);
  if_nil_eval(string) { return 0; }
  size_t string_index = Xen_Vector_Size(blocks->consts->c_instances);
  if (!vm_consts_push_instance(blocks->consts, string)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  if (!ir_add_load_const(block->instr_array, out_reg, string_index)) { return 0; }
  return 1;
}

int compile_literal(int out_reg, block_list_ptr blocks, block_node_ptr block,
                    AST_Node_t *ast) {
  int literal_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, ast->literal.value)) { return 0; }
  if (!ir_add_load_instance(block->instr_array, out_reg, literal_index)) return 0;
  return 1;
}

int compile_property(int out_reg, block_list_ptr blocks, block_node_ptr block,
                     AST_Node_t *ast) {
  int prop_name_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, ast->property.value)) return 0;
  if (!ir_add_load_prop(block->instr_array, out_reg, prop_name_index)) return 0;
  return 1;
}

int compile_assignment(block_list_ptr blocks, block_node_ptr block, AST_Node_t *ast) {
  if (ast->assignment.operator== Xen_Assignment) {
    int name_index = Xen_Vector_Size(blocks->consts->c_names);
    if (!vm_consts_push_name(blocks->consts, ast->assignment.lhs)) { return 0; }
    if (!ir_add_load_name(block->instr_array, 0, name_index)) { return 0; }
    if (ast->assignment.rhs->ast_type == AST_STRING) {
      if (!compile_string(1, blocks, block, ast->assignment.rhs)) { return 0; }
    } else {
      return 0;
    }
    if (!ir_add_make_instance(block->instr_array, 0, 1)) { return 0; }
    return 1;
  }
  return 0;
}

int compile_cmd(block_list_ptr blocks, block_node_ptr block, AST_Node_t *ast) {
  if (!block || !ast) {
    error("No se pudo compilar el commndo");
    return 0;
  }
  int cmd_name_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, ast->cmd.cmd_name)) return 0;
  if (!ir_add_load_name(block->instr_array, 0, cmd_name_index)) return 0;
  for (int arg_iterator = 0; arg_iterator < ast->cmd.arg_count; arg_iterator++) {
    AST_Node_t *arg = ast->cmd.cmd_args[arg_iterator];
    switch (arg->ast_type) {
    case AST_STRING:
      if (!compile_string(arg_iterator + 1, blocks, block, arg)) return 0;
      break;
    case AST_LITERAL:
      if (!compile_literal(arg_iterator + 1, blocks, block, arg)) return 0;
      break;
    case AST_PROPERTY:
      if (!compile_property(arg_iterator + 1, blocks, block, arg)) return 0;
      break;
    default: return 0;
    }
  }
  if (!ir_add_fun_call(block->instr_array, ast->cmd.arg_count)) return 0;
  return 1;
}
