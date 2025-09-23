#include <stddef.h>
#include <string.h>

#include "ast.h"
#include "ast_compiler.h"
#include "block_list.h"
#include "instance.h"
#include "ir_bytecode.h"
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

int ast_compile(block_list_ptr block_result, block_node_ptr *block, AST_Node_t *ast) {
  if (!ast) return 0;

  for (int i = 0; i < ast->child_count; i++) {
    AST_Node_t *node = ast->children[i];

    if (strcmp(node->name, "empty") == 0)
      continue;
    else if (strcmp(node->name, "string") == 0) {
      if (!compile_string(1, block_result, *block, node)) return 0;
    } else if (strcmp(node->name, "literal") == 0) {
      if (!compile_literal(1, block_result, *block, node)) return 0;
    } else if (strcmp(node->name, "property") == 0) {
      if (!compile_property(1, block_result, *block, node)) return 0;
    } else if (strcmp(node->name, "assignment") == 0) {
      if (!compile_assignment(block_result, *block, node)) return 0;
    } else if (strcmp(node->name, "cmd") == 0) {
      if (!compile_cmd(block_result, *block, node)) return 0;
    } else {
      return 0; // nodo desconocido
    }
  }

  return 1;
}

static int compile_string(int out_reg, block_list_ptr blocks, block_node_ptr block,
                          AST_Node_t *ast) {
  Xen_Instance *string = Xen_String_From_CString(ast->value); // ahora usa ast->value
  if_nil_eval(string) return 0;

  size_t string_index = Xen_Vector_Size(blocks->consts->c_instances);
  if (!vm_consts_push_instance(blocks->consts, string)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return ir_add_load_const(block->instr_array, out_reg, string_index);
}

static int compile_literal(int out_reg, block_list_ptr blocks, block_node_ptr block,
                           AST_Node_t *ast) {
  int literal_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, ast->value)) return 0;
  return ir_add_load_instance(block->instr_array, out_reg, literal_index);
}

static int compile_property(int out_reg, block_list_ptr blocks, block_node_ptr block,
                            AST_Node_t *ast) {
  int prop_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, ast->value)) return 0;
  return ir_add_load_prop(block->instr_array, out_reg, prop_index);
}

static int compile_assignment(block_list_ptr blocks, block_node_ptr block,
                              AST_Node_t *ast) {
  if (ast->child_count != 2) return 0;

  AST_Node_t *lhs = ast->children[0];
  AST_Node_t *rhs = ast->children[1];

  // Push LHS name
  int lhs_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, lhs->value)) return 0;
  if (!ir_add_load_name(block->instr_array, 0, lhs_index)) return 0;

  // Compilar RHS
  if (strcmp(rhs->name, "string") == 0) {
    if (!compile_string(1, blocks, block, rhs)) return 0;
  } else if (strcmp(rhs->name, "literal") == 0) {
    if (!compile_literal(1, blocks, block, rhs)) return 0;
  } else if (strcmp(rhs->name, "property") == 0) {
    if (!compile_property(1, blocks, block, rhs)) return 0;
  } else {
    return 0; // tipo no soportado
  }

  // Aquí puedes manejar distintos operadores
  if (strcmp(ast->value, "=") == 0) {
    return ir_add_make_instance(block->instr_array, 0, 1);
  }
  return 0;
}

static int compile_cmd(block_list_ptr blocks, block_node_ptr block, AST_Node_t *ast) {
  if (!block || !ast) return 0;

  // Nombre del comando en ast->value
  int cmd_name_index = Xen_Vector_Size(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, ast->value)) return 0;
  if (!ir_add_load_name(block->instr_array, 0, cmd_name_index)) return 0;

  for (int i = 0; i < ast->child_count; i++) {
    AST_Node_t *arg = ast->children[i];
    if (strcmp(arg->name, "expr") == 0) {
      AST_Node_t *expr = arg->children[0];
      if (strcmp(expr->name, "primary") == 0) {
        AST_Node_t *primary = expr->children[0];
        if (strcmp(primary->name, "string") == 0) {
          if (!compile_string(i + 1, blocks, block, primary)) return 0;
        } else if (strcmp(primary->name, "literal") == 0) {
          if (!compile_literal(i + 1, blocks, block, primary)) return 0;
        } else if (strcmp(primary->name, "property") == 0) {
          if (!compile_property(i + 1, blocks, block, primary)) return 0;
        } else {
          return 0; // tipo no soportado
        }
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }

  return ir_add_fun_call(block->instr_array, ast->child_count);
}
