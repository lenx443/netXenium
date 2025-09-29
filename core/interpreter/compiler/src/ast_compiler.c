#include <stddef.h>
#include <string.h>

#include "ast_compiler.h"
#include "block_list.h"
#include "instance.h"
#include "ir_bytecode.h"
#include "vm_consts.h"
#include "xen_ast.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

#define error(msg, ...) log_add(NULL, ERROR, "AST Compiler", msg, ##__VA_ARGS__)
#define info(msg, ...) log_add(NULL, INFO, "AST Compiler", msg, ##__VA_ARGS__)

static int compile_string(int, block_list_ptr, block_node_ptr, Xen_Instance *);
static int compile_literal(int, block_list_ptr, block_node_ptr, Xen_Instance *);
static int compile_property(int, block_list_ptr, block_node_ptr, Xen_Instance *);
static int compile_assignment(block_list_ptr, block_node_ptr, Xen_Instance *);
static int compile_cmd(block_list_ptr, block_node_ptr, Xen_Instance *);

int ast_compile(block_list_ptr block_result, block_node_ptr *block, Xen_Instance *ast) {
  if (!ast) return 0;

  Xen_Instance *ast_children = Xen_AST_Node_Children(ast);
  for (size_t i = 0; i < Xen_SIZE(ast_children); i++) {
    Xen_Instance *node = Xen_Vector_Peek_Index(ast_children, i);

    if (strcmp(Xen_AST_Node_Name(node), "empty") == 0)
      continue;
    else if (strcmp(Xen_AST_Node_Name(node), "string") == 0) {
      if (!compile_string(1, block_result, *block, node)) {
        Xen_DEL_REF(ast_children);
        return 0;
      }
    } else if (strcmp(Xen_AST_Node_Name(node), "literal") == 0) {
      if (!compile_literal(1, block_result, *block, node)) {
        Xen_DEL_REF(ast_children);
        return 0;
      }
    } else if (strcmp(Xen_AST_Node_Name(node), "property") == 0) {
      if (!compile_property(1, block_result, *block, node)) {
        Xen_DEL_REF(ast_children);
        return 0;
      }
    } else if (strcmp(Xen_AST_Node_Name(node), "assignment") == 0) {
      if (!compile_assignment(block_result, *block, node)) {
        Xen_DEL_REF(ast_children);
        return 0;
      }
    } else if (strcmp(Xen_AST_Node_Name(node), "cmd") == 0) {
      if (!compile_cmd(block_result, *block, node)) {
        Xen_DEL_REF(ast_children);
        return 0;
      }
    } else {
      Xen_DEL_REF(ast_children);
      return 0; // nodo desconocido
    }
  }
  Xen_DEL_REF(ast_children);
  return 1;
}

static int compile_string(int out_reg, block_list_ptr blocks, block_node_ptr block,
                          Xen_Instance *ast) {
  Xen_Instance *string = Xen_String_From_CString(Xen_AST_Node_Value(ast));
  if_nil_eval(string) return 0;

  size_t string_index = Xen_SIZE(blocks->consts->c_instances);
  if (!vm_consts_push_instance(blocks->consts, string)) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  return ir_add_load_const(block->instr_array, out_reg, string_index);
}

static int compile_literal(int out_reg, block_list_ptr blocks, block_node_ptr block,
                           Xen_Instance *ast) {
  int literal_index = Xen_SIZE(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, Xen_AST_Node_Value(ast))) return 0;
  return ir_add_load_instance(block->instr_array, out_reg, literal_index);
}

static int compile_property(int out_reg, block_list_ptr blocks, block_node_ptr block,
                            Xen_Instance *ast) {
  int prop_index = Xen_SIZE(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, Xen_AST_Node_Value(ast))) return 0;
  return ir_add_load_prop(block->instr_array, out_reg, prop_index);
}

static int compile_assignment(block_list_ptr blocks, block_node_ptr block,
                              Xen_Instance *ast) {
  Xen_Instance *children = Xen_AST_Node_Children(ast);
  if (Xen_SIZE(children) != 2) return 0;

  Xen_Instance *lhs = Xen_Vector_Peek_Index(children, 0);
  Xen_Instance *rhs = Xen_Vector_Peek_Index(children, 1);

  // Push LHS name
  int lhs_index = Xen_SIZE(blocks->consts->c_names);
  if (strcmp(Xen_AST_Node_Name(lhs), "literal") == 0) {
    if (!vm_consts_push_name(blocks->consts, Xen_AST_Node_Value(lhs))) {
      Xen_DEL_REF(children);
      return 0;
    }
    if (!ir_add_load_name(block->instr_array, 0, lhs_index)) {
      Xen_DEL_REF(children);
      return 0;
    }
  } else {
    Xen_DEL_REF(children);
    return 0;
  }

  // Compilar RHS
  if (strcmp(Xen_AST_Node_Name(rhs), "string") == 0) {
    if (!compile_string(1, blocks, block, rhs)) {
      Xen_DEL_REF(children);
      return 0;
    }
  } else if (strcmp(Xen_AST_Node_Name(rhs), "literal") == 0) {
    if (!compile_literal(1, blocks, block, rhs)) {
      Xen_DEL_REF(children);
      return 0;
    }
  } else if (strcmp(Xen_AST_Node_Name(rhs), "property") == 0) {
    if (!compile_property(1, blocks, block, rhs)) {
      Xen_DEL_REF(children);
      return 0;
    }
  } else {
    Xen_DEL_REF(children);
    return 0; // tipo no soportado
  }
  Xen_DEL_REF(children);

  // Aquí puedes manejar distintos operadores
  if (strcmp(Xen_AST_Node_Value(ast), "=") == 0) {
    return ir_add_make_instance(block->instr_array, 0, 1);
  }
  return 0;
}

static int compile_cmd(block_list_ptr blocks, block_node_ptr block, Xen_Instance *ast) {
  if (!block || !ast) return 0;

  // Nombre del comando en ast->value
  int cmd_name_index = Xen_SIZE(blocks->consts->c_names);
  if (!vm_consts_push_name(blocks->consts, Xen_AST_Node_Value(ast))) return 0;
  if (!ir_add_load_name(block->instr_array, 0, cmd_name_index)) return 0;

  Xen_Instance *children = Xen_AST_Node_Children(ast);
  for (size_t i = 0; i < Xen_SIZE(children); i++) {
    Xen_Instance *arg = Xen_Vector_Peek_Index(children, i);
    if (strcmp(Xen_AST_Node_Name(arg), "expr") == 0) {
      Xen_Instance *expr = Xen_AST_Node_Get_Child(arg, 0);
      if (strcmp(Xen_AST_Node_Name(expr), "primary") == 0) {
        Xen_Instance *primary = Xen_AST_Node_Get_Child(expr, 0);
        if (strcmp(Xen_AST_Node_Name(primary), "string") == 0) {
          if (!compile_string(i + 1, blocks, block, primary)) {
            Xen_DEL_REF(primary);
            Xen_DEL_REF(expr);
            Xen_DEL_REF(children);
            return 0;
          }
        } else if (strcmp(Xen_AST_Node_Name(primary), "literal") == 0) {
          if (!compile_literal(i + 1, blocks, block, primary)) {
            Xen_DEL_REF(primary);
            Xen_DEL_REF(expr);
            Xen_DEL_REF(children);
            return 0;
          }
        } else if (strcmp(Xen_AST_Node_Name(primary), "property") == 0) {
          if (!compile_property(i + 1, blocks, block, primary)) {
            Xen_DEL_REF(primary);
            Xen_DEL_REF(expr);
            Xen_DEL_REF(children);
            return 0;
          }
        } else {
          Xen_DEL_REF(primary);
          Xen_DEL_REF(expr);
          Xen_DEL_REF(children);
          return 0; // tipo no soportado
        }
        Xen_DEL_REF(primary);
      } else {
        Xen_DEL_REF(expr);
        Xen_DEL_REF(children);
        return 0;
      }
      Xen_DEL_REF(expr);
    } else {
      Xen_DEL_REF(children);
      return 0;
    }
  }
  if (!ir_add_fun_call(block->instr_array, Xen_SIZE(children))) {
    Xen_DEL_REF(children);
    return 0;
  }
  Xen_DEL_REF(children);

  return 1;
}
