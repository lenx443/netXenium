#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bc_instruct.h"
#include "bytecode.h"
#include "compiler.h"
#include "instance.h"
#include "lexer.h"
#include "operators.h"
#include "parser.h"
#include "program_code.h"
#include "vm.h"
#include "vm_consts.h"
#include "vm_def.h"
#include "vm_instructs.h"
#include "xen_ast.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_typedefs.h"

CALLABLE_ptr compiler(const char* text_code) {
  if (!text_code) {
    return NULL;
  }
  Lexer lexer = {text_code, 0};
  Parser parser = {&lexer, {0, "\0"}};
  parser_next(&parser);
  Xen_Instance* ast_program = parser_program(&parser);
  if (!ast_program) {
#ifndef NDEBUG
    printf("Parser Error\n");
#endif
    return 0;
  }
#ifndef NDEBUG
  Xen_AST_Node_Print(ast_program);
#endif
  ProgramCode_t pc;
  pc.code = bc_new();
  if (!pc.code) {
    Xen_DEL_REF(ast_program);
    return 0;
  }
  pc.consts = vm_consts_new();
  if (!pc.consts) {
    Xen_DEL_REF(ast_program);
    bc_free(pc.code);
    return 0;
  }
  if (!ast_compile(&pc, ast_program)) {
    Xen_DEL_REF(ast_program);
    vm_consts_free(pc.consts);
    bc_free(pc.code);
    return 0;
  }
  Xen_DEL_REF(ast_program);
#ifndef NDEBUG
  bc_print(pc);
#endif
  vm_ctx_clear(vm->root_context);
  program_stack_depth(&pc);
  CALLABLE_ptr code = callable_new_code(pc);
  if (!code) {
    vm_consts_free(pc.consts);
    bc_free(pc.code);
    return 0;
  }
  return code;
}

static int compile_program(ProgramCode_t*, Xen_Instance*);

static int compile_statement_list(ProgramCode_t*, Xen_Instance*);
static int compile_statement(ProgramCode_t*, Xen_Instance*);

static int compile_expr(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_string(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_number(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_literal(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_property(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_parent(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_suffix(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_suffix_call(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_suffix_index(ProgramCode_t*, Xen_Instance*);
static int compile_expr_primary_suffix_attr(ProgramCode_t*, Xen_Instance*);
static int compile_expr_unary(ProgramCode_t*, Xen_Instance*);
static int compile_expr_binary(ProgramCode_t*, Xen_Instance*);
static int compile_expr_list(ProgramCode_t*, Xen_Instance*);

static int compile_assignment(ProgramCode_t*, Xen_Instance*);
static int compile_assignment_expr(ProgramCode_t*, Xen_Instance*);
static int compile_assignment_expr_primary(ProgramCode_t*, Xen_Instance*);
static int compile_assignment_expr_primary_literal(ProgramCode_t*,
                                                   Xen_Instance*);
static int compile_assignment_expr_primary_property(ProgramCode_t*,
                                                    Xen_Instance*);
static int compile_assignment_expr_primary_parent(ProgramCode_t*,
                                                  Xen_Instance*);
static int compile_assignment_expr_primary_suffix(ProgramCode_t*,
                                                  Xen_Instance*);
static int compile_assignment_expr_primary_suffix_index(ProgramCode_t*,
                                                        Xen_Instance*);
static int compile_assignment_expr_primary_suffix_attr(ProgramCode_t*,
                                                       Xen_Instance*);

static int compile_if_statement(ProgramCode_t*, Xen_Instance*);

int compile_program(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* stmt_list = Xen_AST_Node_Get_Child(node, 0);
  if (!stmt_list) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(stmt_list, "StatementList") == 0) {
    if (!compile_statement_list(code, stmt_list)) {
      Xen_DEL_REF(stmt_list);
      return 0;
    }
  } else {
    Xen_DEL_REF(stmt_list);
    return 0;
  }
  Xen_DEL_REF(stmt_list);
  return 1;
}

int compile_statement_list(ProgramCode_t* code, Xen_Instance* node) {
  for (Xen_size_t idx = 0; idx < Xen_AST_Node_Children_Size(node); idx++) {
    Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, idx);
    if (!compile_statement(code, stmt)) {
      Xen_DEL_REF(stmt);
      return 0;
    }
    Xen_DEL_REF(stmt);
  }
  return 1;
}

int compile_statement(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, 0);
  if (!stmt) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(stmt, "Expr") == 0) {
    if (!compile_expr(code, stmt)) {
      Xen_DEL_REF(stmt);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(stmt, "Assignment") == 0) {
    if (!compile_assignment(code, stmt)) {
      Xen_DEL_REF(stmt);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(stmt, "IfStatement") == 0) {
    if (!compile_if_statement(code, stmt)) {
      Xen_DEL_REF(stmt);
      return 0;
    }
  } else {
    Xen_DEL_REF(stmt);
    return 0;
  }
  Xen_DEL_REF(stmt);
  return 1;
}

int compile_expr(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
    if (!compile_expr_primary(code, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "Unary") == 0) {
    if (!compile_expr_unary(code, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "Binary") == 0) {
    if (!compile_expr_binary(code, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "List") == 0) {
    if (!compile_expr_list(code, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else {
    Xen_DEL_REF(expr);
    return 0;
  }
  Xen_DEL_REF(expr);
  return 1;
}

int compile_expr_primary(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
  if (!primary) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(primary, "String") == 0) {
    if (!compile_expr_primary_string(code, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Number") == 0) {
    if (!compile_expr_primary_number(code, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
    if (!compile_expr_primary_literal(code, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
    if (!compile_expr_primary_property(code, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
    if (!compile_expr_primary_parent(code, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else {
    Xen_DEL_REF(primary);
    return 0;
  }
  Xen_DEL_REF(primary);
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(suffix, "Suffix") == 0) {
      if (!compile_expr_primary_suffix(code, suffix)) {
        Xen_DEL_REF(suffix);
        return 0;
      }
    } else {
      Xen_DEL_REF(suffix);
      return 0;
    }
    Xen_DEL_REF(suffix);
  }
  return 1;
}

int compile_expr_primary_string(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* string = Xen_String_From_CString(Xen_AST_Node_Value(node));
  if (!string) {
    return 0;
  }
  Xen_ssize_t co_idx = vm_consts_push_instance(code->consts, string);
  if (co_idx == -1) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  if (bc_emit(code->code, -1, PUSH, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_number(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* number = Xen_Number_From_CString(Xen_AST_Node_Value(node), 0);
  if (!number) {
    return 0;
  }
  Xen_ssize_t co_idx = vm_consts_push_instance(code->consts, number);
  if (co_idx == -1) {
    Xen_DEL_REF(number);
    return 0;
  }
  Xen_DEL_REF(number);
  if (bc_emit(code->code, -1, PUSH, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_literal(ProgramCode_t* code, Xen_Instance* node) {
  Xen_ssize_t co_idx =
      vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (bc_emit(code->code, -1, LOAD, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_property(ProgramCode_t* code, Xen_Instance* node) {
  Xen_ssize_t co_idx =
      vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (bc_emit(code->code, -1, LOAD_PROP, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_parent(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Expr") == 0) {
    if (!compile_expr(code, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else {
    Xen_DEL_REF(expr);
    return 0;
  }
  Xen_DEL_REF(expr);
  return 1;
}

int compile_expr_primary_suffix(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 0);
  if (!suffix) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(suffix, "Call") == 0) {
    if (!compile_expr_primary_suffix_call(code, suffix)) {
      Xen_DEL_REF(suffix);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Index") == 0) {
    if (!compile_expr_primary_suffix_index(code, suffix)) {
      Xen_DEL_REF(suffix);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
    if (!compile_expr_primary_suffix_attr(code, suffix)) {
      Xen_DEL_REF(suffix);
      return 0;
    }
  } else {
    Xen_DEL_REF(suffix);
    return 0;
  }
  Xen_DEL_REF(suffix);
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* suffix2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(suffix2, "Suffix") == 0) {
      if (!compile_expr_primary_suffix(code, suffix2)) {
        Xen_DEL_REF(suffix2);
        return 0;
      }
    } else {
      Xen_DEL_REF(suffix2);
      return 0;
    }
    Xen_DEL_REF(suffix2);
  }
  return 1;
}

int compile_expr_primary_suffix_call(ProgramCode_t* code, Xen_Instance* node) {
  for (Xen_size_t idx = Xen_AST_Node_Children_Size(node); idx > 0; idx--) {
    Xen_Instance* arg = Xen_AST_Node_Get_Child(node, idx - 1);
    if (Xen_AST_Node_Name_Cmp(arg, "Expr") == 0) {
      if (!compile_expr(code, arg)) {
        Xen_DEL_REF(arg);
        return 0;
      }
    } else {
      Xen_DEL_REF(arg);
      return 0;
    }
    Xen_DEL_REF(arg);
  }
  if (bc_emit(code->code, -1, CALL,
              (uint8_t)Xen_AST_Node_Children_Size(node)) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_suffix_index(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* index = Xen_AST_Node_Get_Child(node, 0);
  if (!index) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(index, "Expr") == 0) {
    if (!compile_expr(code, index)) {
      Xen_DEL_REF(index);
      return 0;
    }
  } else {
    Xen_DEL_REF(index);
    return 0;
  }
  Xen_DEL_REF(index);
  if (bc_emit(code->code, -1, LOAD_INDEX, 0) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_suffix_attr(ProgramCode_t* code, Xen_Instance* node) {
  Xen_ssize_t co_idx =
      vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (bc_emit(code->code, -1, LOAD_ATTR, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_unary(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* val = Xen_AST_Node_Get_Child(node, 0);
  if (!val) {
    return 0;
  }
  if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
    if (Xen_AST_Node_Name_Cmp(val, "Primary") == 0) {
      if (!compile_expr_primary(code, val)) {
        Xen_DEL_REF(val);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(val, "Unary") == 0) {
      if (!compile_expr_unary(code, val)) {
        Xen_DEL_REF(val);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(val, "Binary") == 0) {
      if (!compile_expr_binary(code, val)) {
        Xen_DEL_REF(val);
        return 0;
      }
    } else {
      Xen_DEL_REF(val);
      return 0;
    }
  } else {
    if (Xen_AST_Node_Name_Cmp(val, "Primary") == 0) {
      if (!compile_expr_primary(code, val)) {
        Xen_DEL_REF(val);
        return 0;
      }
    } else {
      Xen_DEL_REF(val);
      return 0;
    }
  }
  Xen_DEL_REF(val);
  if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
    if (bc_emit(code->code, -1, UNARY_POSITIVE, 0) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
    if (bc_emit(code->code, -1, UNARY_NEGATIVE, 0) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
    if (bc_emit(code->code, -1, UNARY_NOT, 0) == -1) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_expr_binary(ProgramCode_t* code, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) != 2) {
    return 0;
  }
  if (Xen_AST_Node_Value_Cmp(node, "and") == 0) {
    Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
      if (!compile_expr_primary(code, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
      if (!compile_expr_unary(code, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
      if (!compile_expr_binary(code, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr1);
      return 0;
    }
    Xen_DEL_REF(expr1);
    if (bc_emit(code->code, -1, COPY, 0) == -1) {
      return 0;
    }
    Xen_ssize_t jump_offset = bc_emit(code->code, -1, JUMP_IF_FALSE, 0xff);
    if (jump_offset < 0) {
      return 0;
    }
    if (bc_emit(code->code, -1, POP, 0) == -1) {
      return 0;
    }
    Xen_Instance* expr2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(expr2, "Primary") == 0) {
      if (!compile_expr_primary(code, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
      if (!compile_expr_unary(code, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
      if (!compile_expr_binary(code, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr2);
      return 0;
    }
    Xen_DEL_REF(expr2);
    bc_emit(code->code, jump_offset, JUMP_IF_FALSE, code->code->bc_size);
    if (bc_emit(code->code, -1, NOP, 0) == -1) {
      return 0;
    }
    return 1;
  }
  if (Xen_AST_Node_Value_Cmp(node, "or") == 0) {
    Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
      if (!compile_expr_primary(code, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
      if (!compile_expr_unary(code, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
      if (!compile_expr_binary(code, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr1);
      return 0;
    }
    Xen_DEL_REF(expr1);
    if (bc_emit(code->code, -1, COPY, 0) == -1) {
      return 0;
    }
    Xen_ssize_t jump_offset = bc_emit(code->code, -1, JUMP_IF_TRUE, 0xff);
    if (jump_offset < 0) {
      return 0;
    }
    if (bc_emit(code->code, -1, POP, 0) == -1) {
      return 0;
    }
    Xen_Instance* expr2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(expr2, "Primary") == 0) {
      if (!compile_expr_primary(code, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
      if (!compile_expr_unary(code, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
      if (!compile_expr_binary(code, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr2);
      return 0;
    }
    Xen_DEL_REF(expr2);
    bc_emit(code->code, jump_offset, JUMP_IF_TRUE, code->code->bc_size);
    if (bc_emit(code->code, -1, NOP, 0) == -1) {
      return 0;
    }
    return 1;
  }
  Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
    if (!compile_expr_primary(code, expr1)) {
      Xen_DEL_REF(expr1);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
    if (!compile_expr_unary(code, expr1)) {
      Xen_DEL_REF(expr1);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
    if (!compile_expr_binary(code, expr1)) {
      Xen_DEL_REF(expr1);
      return 0;
    }
  } else {
    Xen_DEL_REF(expr1);
    return 0;
  }
  Xen_DEL_REF(expr1);
  Xen_Instance* expr2 = Xen_AST_Node_Get_Child(node, 1);
  if (Xen_AST_Node_Name_Cmp(expr2, "Primary") == 0) {
    if (!compile_expr_primary(code, expr2)) {
      Xen_DEL_REF(expr2);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
    if (!compile_expr_unary(code, expr2)) {
      Xen_DEL_REF(expr2);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
    if (!compile_expr_binary(code, expr2)) {
      Xen_DEL_REF(expr2);
      return 0;
    }
  } else {
    Xen_DEL_REF(expr2);
    return 0;
  }
  Xen_DEL_REF(expr2);
  if (Xen_AST_Node_Value_Cmp(node, "**") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_POW) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "*") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_MUL) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "/") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_DIV) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "%") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_MOD) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_ADD) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_SUB) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "<") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_LT) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "<=") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_LE) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, ">") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_GT) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, ">=") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_GE) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "==") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_EQ) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "!=") == 0) {
    if (bc_emit(code->code, -1, BINARYOP, (uint8_t)Xen_OPR_NE) == -1) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_expr_list(ProgramCode_t* code, Xen_Instance* node) {
  return 0;
}

int compile_assignment(ProgramCode_t* code, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) != 2) {
    return 0;
  }
  Xen_Instance* rhs = Xen_AST_Node_Get_Child(node, 1);
  if (Xen_AST_Node_Name_Cmp(rhs, "Expr") != 0) {
    Xen_DEL_REF(rhs);
    return 0;
  }
  if (!compile_expr(code, rhs)) {
    Xen_DEL_REF(rhs);
    return 0;
  }
  Xen_DEL_REF(rhs);
  Xen_Instance* lhs = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(lhs, "Expr") != 0) {
    Xen_DEL_REF(lhs);
    return 0;
  }
  if (!compile_assignment_expr(code, lhs)) {
    Xen_DEL_REF(lhs);
    return 0;
  }
  Xen_DEL_REF(lhs);
  return 1;
}

int compile_assignment_expr(ProgramCode_t* code, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
    if (!compile_assignment_expr_primary(code, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else {
    Xen_DEL_REF(expr);
    return 0;
  }
  Xen_DEL_REF(expr);
  return 1;
}

int compile_assignment_expr_primary(ProgramCode_t* code, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(primary, "String") == 0) {
      if (!compile_expr_primary_string(code, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Number") == 0) {
      if (!compile_expr_primary_number(code, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
      if (!compile_expr_primary_literal(code, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
      if (!compile_expr_primary_property(code, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
      if (!compile_expr_primary_parent(code, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else {
      Xen_DEL_REF(primary);
      return 0;
    }
    Xen_DEL_REF(primary);
    Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(suffix, "Suffix") == 0) {
      if (!compile_assignment_expr_primary_suffix(code, suffix)) {
        Xen_DEL_REF(suffix);
        return 0;
      }
    } else {
      Xen_DEL_REF(suffix);
      return 0;
    }
    Xen_DEL_REF(suffix);
    return 1;
  }
  Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
  if (!primary) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
    if (!compile_assignment_expr_primary_literal(code, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
    if (!compile_assignment_expr_primary_property(code, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
    if (!compile_assignment_expr_primary_parent(code, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else {
    Xen_DEL_REF(primary);
    return 0;
  }
  Xen_DEL_REF(primary);
  return 1;
}

int compile_assignment_expr_primary_literal(ProgramCode_t* code,
                                            Xen_Instance* node) {
  Xen_ssize_t co_idx =
      vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (bc_emit(code->code, -1, STORE, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_property(ProgramCode_t* code,
                                             Xen_Instance* node) {
  Xen_ssize_t co_idx =
      vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (bc_emit(code->code, -1, STORE_PROP, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_parent(ProgramCode_t* code,
                                           Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Expr") == 0) {
    if (!compile_assignment_expr(code, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else {
    Xen_DEL_REF(expr);
    return 0;
  }
  Xen_DEL_REF(expr);
  return 1;
  return 0;
}

int compile_assignment_expr_primary_suffix(ProgramCode_t* code,
                                           Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(suffix, "Call") == 0) {
      if (!compile_expr_primary_suffix_call(code, suffix)) {
        Xen_DEL_REF(suffix);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(suffix, "Index") == 0) {
      if (!compile_expr_primary_suffix_index(code, suffix)) {
        Xen_DEL_REF(suffix);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
      if (!compile_expr_primary_suffix_attr(code, suffix)) {
        Xen_DEL_REF(suffix);
        return 0;
      }
    } else {
      Xen_DEL_REF(suffix);
      return 0;
    }
    Xen_DEL_REF(suffix);
    Xen_Instance* suffix2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(suffix2, "Suffix") == 0) {
      if (!compile_assignment_expr_primary_suffix(code, suffix2)) {
        Xen_DEL_REF(suffix2);
        return 0;
      }
    } else {
      Xen_DEL_REF(suffix2);
      return 0;
    }
    Xen_DEL_REF(suffix2);
    return 1;
  }
  Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 0);
  if (!suffix) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(suffix, "Index") == 0) {
    if (!compile_assignment_expr_primary_suffix_index(code, suffix)) {
      Xen_DEL_REF(suffix);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
    if (!compile_assignment_expr_primary_suffix_attr(code, suffix)) {
      Xen_DEL_REF(suffix);
      return 0;
    }
  } else {
    Xen_DEL_REF(suffix);
    return 0;
  }
  Xen_DEL_REF(suffix);
  return 1;
}

int compile_assignment_expr_primary_suffix_index(ProgramCode_t* code,
                                                 Xen_Instance* node) {
  Xen_Instance* index = Xen_AST_Node_Get_Child(node, 0);
  if (!index) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(index, "Expr") == 0) {
    if (!compile_expr(code, index)) {
      Xen_DEL_REF(index);
      return 0;
    }
  } else {
    Xen_DEL_REF(index);
    return 0;
  }
  Xen_DEL_REF(index);
  if (bc_emit(code->code, -1, STORE_INDEX, 0) == -1) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_suffix_attr(ProgramCode_t* code,
                                                Xen_Instance* node) {
  Xen_ssize_t co_idx =
      vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (bc_emit(code->code, -1, STORE_ATTR, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_if_statement(ProgramCode_t* code, Xen_Instance* node) {
  return 0;
}

int ast_compile(ProgramCode_t* code, Xen_Instance* ast) {
  return compile_program(code, ast);
  /*
    struct Emit_Value {
      Xen_ssize_t offset;
      uint8_t opcode, oparg;
    } emit_value = {0, 0, 0};
    struct Frame {
      Xen_Instance* node;
      Xen_size_t passes;
      uint8_t mode;
      uintptr_t stack_data_1;
    } stack[1024];
    typedef struct Emit_Value Emit_Value;
    typedef struct Frame Frame;
    Xen_ssize_t Emit_Result = 0;
  #define FRAME1(node) (Frame){node, 0, 0, 0}
  #define FRAME2(node, mode) (Frame){node, 0, mode, 0}

  #define MODE_EXPR_ASSIGNMENT_LHS 1
    size_t sp = 0;
    stack[sp++] = FRAME1(ast);
    Frame Error = FRAME1(Xen_AST_Node_New("CompilerError", NULL));
    if (!Error.node) {
      return 0;
    }
    Frame Emit = FRAME1(Xen_AST_Node_New("CompilerEmit", NULL));
    if (!Emit.node) {
      Xen_DEL_REF(Error.node);
      return 0;
    }
    while (sp > 0) {
      Frame* frame = &stack[sp - 1];
      Xen_Instance* node = frame->node;
      uint8_t mode = frame->mode;
      if (Xen_AST_Node_Name_Cmp(node, "CompilerError") == 0) {
  #ifndef NDEBUG
        printf("Compile Error\n");
  #endif
        Xen_DEL_REF(Emit.node);
        Xen_DEL_REF(Error.node);
        return 0;
      } else if (Xen_AST_Node_Name_Cmp(node, "CompilerEmit") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        if ((Emit_Result = bc_emit(code->code, emit_value.offset,
                                   emit_value.opcode, emit_value.oparg)) ==
  -1) { stack[sp++] = Error; continue;
        }
        frame->passes++;
      } else if (Xen_AST_Node_Name_Cmp(node, "Program") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        Xen_Instance* stmts = Xen_AST_Node_Get_Child(node, 0);
        if (!stmts) {
          stack[sp++] = Error;
          continue;
        }
        if (Xen_AST_Node_Name_Cmp(stmts, "StatementList") != 0 &&
            Xen_AST_Node_Name_Cmp(stmts, "Statement") != 0) {
          Xen_DEL_REF(stmts);
          stack[sp++] = Error;
          continue;
        }
        stack[sp++] = FRAME1(stmts);
        Xen_DEL_REF(stmts);
        frame->passes++;
      } else if (Xen_AST_Node_Name_Cmp(node, "StatementList") == 0) {
        if (frame->passes < Xen_AST_Node_Children_Size(node)) {
          Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, frame->passes++);
          if (Xen_AST_Node_Name_Cmp(stmt, "Statement") != 0) {
            Xen_DEL_REF(stmt);
            stack[sp++] = Error;
            continue;
          }
          stack[sp++] = FRAME1(stmt);
          Xen_DEL_REF(stmt);
          continue;
        }
        --sp;
      } else if (Xen_AST_Node_Name_Cmp(node, "Statement") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, 0);
        if (!stmt) {
          stack[sp++] = Error;
          continue;
        }
        if (Xen_AST_Node_Name_Cmp(stmt, "Expr") != 0 &&
            Xen_AST_Node_Name_Cmp(stmt, "Assignment") != 0 &&
            Xen_AST_Node_Name_Cmp(stmt, "IfStatement") != 0 &&
            Xen_AST_Node_Name_Cmp(stmt, "WhileStatement") != 0 &&
            Xen_AST_Node_Name_Cmp(stmt, "ForStatement") != 0) {
          Xen_DEL_REF(stmt);
          stack[sp++] = Error;
          continue;
        }
        stack[sp++] = FRAME1(stmt);
        Xen_DEL_REF(stmt);
        frame->passes++;
      } else if (Xen_AST_Node_Name_Cmp(node, "Expr") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        if (mode == MODE_EXPR_ASSIGNMENT_LHS) {
          Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
          if (!value) {
            stack[sp++] = Error;
            continue;
          }
          if (Xen_AST_Node_Name_Cmp(value, "Primary") != 0) {
            Xen_DEL_REF(value);
            stack[sp++] = Error;
            continue;
          }
          stack[sp++] = FRAME2(value, MODE_EXPR_ASSIGNMENT_LHS);
          Xen_DEL_REF(value);
          frame->passes++;
        } else {
          Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
          if (!value) {
            stack[sp++] = Error;
            continue;
          }
          if (Xen_AST_Node_Name_Cmp(value, "Primary") != 0 &&
              Xen_AST_Node_Name_Cmp(value, "Unary") != 0 &&
              Xen_AST_Node_Name_Cmp(value, "Binary") != 0 &&
              Xen_AST_Node_Name_Cmp(value, "List") != 0) {
            Xen_DEL_REF(value);
            stack[sp++] = Error;
            continue;
          }
          stack[sp++] = FRAME1(value);
          Xen_DEL_REF(value);
          frame->passes++;
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Primary") == 0) {
        if (mode == MODE_EXPR_ASSIGNMENT_LHS &&
            Xen_AST_Node_Children_Size(node) == 1) {
          switch (frame->passes) {
          case 0: {
            Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
            if (!value) {
              stack[sp++] = Error;
              break;
            }
            if (Xen_AST_Node_Name_Cmp(value, "Literal") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Property") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Parent") != 0) {
              Xen_DEL_REF(value);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME2(value, MODE_EXPR_ASSIGNMENT_LHS);
            Xen_DEL_REF(value);
            frame->passes++;
            break;
          }
          default:
            --sp;
            break;
          }
        } else {
          switch (frame->passes) {
          case 0: {
            Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
            if (!value) {
              stack[sp++] = Error;
              break;
            }
            if (Xen_AST_Node_Name_Cmp(value, "String") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Number") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Literal") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Property") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Parent") != 0) {
              Xen_DEL_REF(value);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(value);
            Xen_DEL_REF(value);
            frame->passes++;
            break;
          }
          case 1: {
            if (Xen_AST_Node_Children_Size(node) == 2) {
              Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 1);
              if (!suffix) {
                stack[sp++] = Error;
                break;
              }
              if (Xen_AST_Node_Name_Cmp(suffix, "Suffix") != 0) {
                Xen_DEL_REF(suffix);
                stack[sp++] = Error;
                break;
              }
              if (mode == MODE_EXPR_ASSIGNMENT_LHS) {
                stack[sp++] = FRAME2(suffix, MODE_EXPR_ASSIGNMENT_LHS);
              } else {
                stack[sp++] = FRAME1(suffix);
              }
              Xen_DEL_REF(suffix);
              frame->passes++;
              break;
            } else if (Xen_AST_Node_Children_Size(node) > 2) {
              stack[sp++] = Error;
              break;
            }
            frame->passes++;
            break;
          }
          default:
            --sp;
            break;
          }
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "String") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        Xen_Instance* value =
  Xen_String_From_CString(Xen_AST_Node_Value(node)); if (!value) { stack[sp++]
  = Error; continue;
        }
        Xen_ssize_t co_idx = vm_consts_push_instance(code->consts, value);
        if (co_idx < 0) {
          Xen_DEL_REF(value);
          stack[sp++] = Error;
          continue;
        }
        Xen_DEL_REF(value);
        emit_value = (Emit_Value){-1, PUSH, (uint8_t)co_idx};
        stack[sp++] = Emit;
        frame->passes++;
      } else if (Xen_AST_Node_Name_Cmp(node, "Number") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        Xen_Instance* value =
            Xen_Number_From_CString(Xen_AST_Node_Value(node), 0);
        if (!value) {
          stack[sp++] = Error;
          continue;
        }
        int co_idx = vm_consts_push_instance(code->consts, value);
        if (co_idx < 0) {
          Xen_DEL_REF(value);
          stack[sp++] = Error;
          continue;
        }
        Xen_DEL_REF(value);
        emit_value = (Emit_Value){-1, PUSH, (uint8_t)co_idx};
        stack[sp++] = Emit;
        frame->passes++;
      } else if (Xen_AST_Node_Name_Cmp(node, "Literal") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        if (mode == MODE_EXPR_ASSIGNMENT_LHS) {
          int co_idx =
              vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
          if (co_idx < 0) {
            stack[sp++] = Error;
            continue;
          }
          emit_value = (Emit_Value){-1, STORE, (uint8_t)co_idx};
          stack[sp++] = Emit;
          frame->passes++;
        } else {
          int co_idx =
              vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
          if (co_idx < 0) {
            stack[sp++] = Error;
            continue;
          }
          emit_value = (Emit_Value){-1, LOAD, (uint8_t)co_idx};
          stack[sp++] = Emit;
          frame->passes++;
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Property") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        if (mode == MODE_EXPR_ASSIGNMENT_LHS) {
          int co_idx =
              vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
          if (co_idx < 0) {
            stack[sp++] = Error;
            continue;
          }
          emit_value = (Emit_Value){-1, STORE_PROP, (uint8_t)co_idx};
          stack[sp++] = Emit;
          frame->passes++;
        } else {
          int co_idx =
              vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
          if (co_idx < 0) {
            stack[sp++] = Error;
            continue;
          }
          emit_value = (Emit_Value){-1, LOAD_PROP, (uint8_t)co_idx};
          stack[sp++] = Emit;
          frame->passes++;
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Parent") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        if (mode == MODE_EXPR_ASSIGNMENT_LHS) {
          Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
          if (!expr) {
            stack[sp++] = Error;
            continue;
          }
          if (Xen_AST_Node_Name_Cmp(expr, "Expr") != 0) {
            Xen_DEL_REF(expr);
            stack[sp++] = Error;
            continue;
          }
          stack[sp++] = FRAME2(expr, MODE_EXPR_ASSIGNMENT_LHS);
          Xen_DEL_REF(expr);
          frame->passes++;
        } else {
          Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
          if (!expr) {
            stack[sp++] = Error;
            continue;
          }
          if (Xen_AST_Node_Name_Cmp(expr, "Expr") != 0) {
            Xen_DEL_REF(expr);
            stack[sp++] = Error;
            continue;
          }
          stack[sp++] = FRAME1(expr);
          Xen_DEL_REF(expr);
          frame->passes++;
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Suffix") == 0) {
        if (mode == MODE_EXPR_ASSIGNMENT_LHS &&
            Xen_AST_Node_Children_Size(node) == 1) {
          switch (frame->passes) {
          case 0: {
            Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
            if (!value) {
              stack[sp++] = Error;
              continue;
            }
            if (Xen_AST_Node_Name_Cmp(value, "Index") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Attr") != 0) {
              Xen_DEL_REF(value);
              stack[sp++] = Error;
              continue;
            }
            stack[sp++] = FRAME2(value, MODE_EXPR_ASSIGNMENT_LHS);
            Xen_DEL_REF(value);
            frame->passes++;
            break;
          }
          default:
            --sp;
            break;
          }
        } else {
          switch (frame->passes) {
          case 0: {
            Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
            if (!value) {
              stack[sp++] = Error;
              continue;
            }
            if (Xen_AST_Node_Name_Cmp(value, "Call") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Index") != 0 &&
                Xen_AST_Node_Name_Cmp(value, "Attr") != 0) {
              Xen_DEL_REF(value);
              stack[sp++] = Error;
              continue;
            }
            stack[sp++] = FRAME1(value);
            Xen_DEL_REF(value);
            frame->passes++;
            break;
          }
          case 1: {
            if (Xen_AST_Node_Children_Size(node) == 2) {
              Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 1);
              if (!suffix) {
                stack[sp++] = Error;
                break;
              }
              if (Xen_AST_Node_Name_Cmp(suffix, "Suffix") != 0) {
                Xen_DEL_REF(suffix);
                stack[sp++] = Error;
                break;
              }
              if (mode == MODE_EXPR_ASSIGNMENT_LHS) {
                stack[sp++] = FRAME2(suffix, MODE_EXPR_ASSIGNMENT_LHS);
              } else {
                stack[sp++] = FRAME1(suffix);
              }
              Xen_DEL_REF(suffix);
              frame->passes++;
              break;
            } else if (Xen_AST_Node_Children_Size(node) > 2) {
              stack[sp++] = Error;
              break;
            }
          }
          default:
            --sp;
            break;
          }
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Call") == 0) {
        switch (frame->passes) {
        case 0:
          for (Xen_size_t idx = 0; idx < Xen_AST_Node_Children_Size(node);
               idx++) {
            Xen_Instance* arg = Xen_AST_Node_Get_Child(node, idx);
            if (Xen_AST_Node_Name_Cmp(arg, "Expr") != 0) {
              Xen_DEL_REF(arg);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(arg);
            Xen_DEL_REF(arg);
          }
          frame->passes++;
          break;
        case 1:
          emit_value =
              (Emit_Value){-1, CALL,
  (uint8_t)Xen_AST_Node_Children_Size(node)}; stack[sp++] = Emit;
          frame->passes++;
          break;
        default:
          --sp;
          break;
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Index") == 0) {
        if (mode == MODE_EXPR_ASSIGNMENT_LHS) {
          switch (frame->passes) {
          case 0:
            if (Xen_AST_Node_Children_Size(node) != 1) {
              stack[sp++] = Error;
              break;
            }
            Xen_Instance* index = Xen_AST_Node_Get_Child(node, 0);
            if (Xen_AST_Node_Name_Cmp(index, "Expr") != 0) {
              Xen_DEL_REF(index);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(index);
            Xen_DEL_REF(index);
            frame->passes++;
            break;
          case 1:
            emit_value = (Emit_Value){-1, STORE_INDEX, 0};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          default:
            --sp;
            break;
          }
        } else {
          switch (frame->passes) {
          case 0:
            if (Xen_AST_Node_Children_Size(node) != 1) {
              stack[sp++] = Error;
              break;
            }
            Xen_Instance* index = Xen_AST_Node_Get_Child(node, 0);
            if (Xen_AST_Node_Name_Cmp(index, "Expr") != 0) {
              Xen_DEL_REF(index);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(index);
            Xen_DEL_REF(index);
            frame->passes++;
            break;
          case 1:
            emit_value = (Emit_Value){-1, LOAD_INDEX, 0};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          default:
            --sp;
            break;
          }
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Attr") == 0) {
        if (frame->passes > 0) {
          --sp;
          continue;
        }
        if (mode == MODE_EXPR_ASSIGNMENT_LHS) {
          Xen_ssize_t co_idx =
              vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
          if (co_idx < 0) {
            stack[sp++] = Error;
            continue;
          }
          emit_value = (Emit_Value){-1, STORE_ATTR, co_idx};
          stack[sp++] = Emit;
          frame->passes++;
        } else {
          Xen_ssize_t co_idx =
              vm_consts_push_name(code->consts, Xen_AST_Node_Value(node));
          if (co_idx < 0) {
            stack[sp++] = Error;
            continue;
          }
          emit_value = (Emit_Value){-1, LOAD_ATTR, co_idx};
          stack[sp++] = Emit;
          frame->passes++;
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Unary") == 0) {
        switch (frame->passes) {
        case 0:
          if (Xen_AST_Node_Children_Size(node) != 1) {
            stack[sp++] = Error;
            break;
          }
          Xen_Instance* val = Xen_AST_Node_Get_Child(node, 0);
          if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
            if (Xen_AST_Node_Name_Cmp(val, "Primary") != 0 &&
                Xen_AST_Node_Name_Cmp(val, "Unary") != 0 &&
                Xen_AST_Node_Name_Cmp(val, "Binary") != 0) {
              Xen_DEL_REF(val);
              stack[sp++] = Error;
              break;
            }
          } else if (Xen_AST_Node_Name_Cmp(val, "Primary") != 0) {
            Xen_DEL_REF(val);
            stack[sp++] = Error;
            break;
          }
          stack[sp++] = FRAME1(val);
          Xen_DEL_REF(val);
          frame->passes++;
          break;
        case 1:
          if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
            emit_value = (Emit_Value){-1, UNARY_POSITIVE, 0};
            stack[sp++] = Emit;
            frame->passes++;
          } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
            emit_value = (Emit_Value){-1, UNARY_NEGATIVE, 0};
            stack[sp++] = Emit;
            frame->passes++;
          } else if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
            emit_value = (Emit_Value){-1, UNARY_NOT, 0};
            stack[sp++] = Emit;
            frame->passes++;
          } else {
            stack[sp++] = Error;
          }
          break;
        default:
          --sp;
          break;
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Binary") == 0) {
        if (Xen_AST_Node_Value_Cmp(node, "and") == 0) {
          switch (frame->passes) {
          case 0: {
            if (Xen_AST_Node_Children_Size(node) != 2) {
              stack[sp++] = Error;
              break;
            }
            Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
            if (Xen_AST_Node_Name_Cmp(expr, "Binary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Unary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Primary") != 0) {
              Xen_DEL_REF(expr);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(expr);
            Xen_DEL_REF(expr);
            frame->passes++;
            break;
          }
          case 1: {
            emit_value = (Emit_Value){-1, COPY, 0};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          case 2: {
            emit_value = (Emit_Value){-1, JUMP_IF_FALSE, 0xFF};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          case 3: {
            frame->stack_data_1 = Emit_Result;
            emit_value = (Emit_Value){-1, POP, 0};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          case 4: {
            Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 1);
            if (Xen_AST_Node_Name_Cmp(expr, "Binary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Unary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Primary") != 0) {
              Xen_DEL_REF(expr);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(expr);
            Xen_DEL_REF(expr);
            frame->passes++;
            break;
          }
          case 5: {
            emit_value = (Emit_Value){frame->stack_data_1, JUMP_IF_FALSE,
                                      code->code->bc_size};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          case 6: {
            emit_value = (Emit_Value){-1, NOP, 0};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          default: {
            --sp;
            break;
          }
          }
        } else if (Xen_AST_Node_Value_Cmp(node, "or") == 0) {
          switch (frame->passes) {
          case 0: {
            if (Xen_AST_Node_Children_Size(node) != 2) {
              stack[sp++] = Error;
              break;
            }
            Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
            if (Xen_AST_Node_Name_Cmp(expr, "Binary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Unary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Primary") != 0) {
              Xen_DEL_REF(expr);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(expr);
            Xen_DEL_REF(expr);
            frame->passes++;
            break;
          }
          case 1: {
            emit_value = (Emit_Value){-1, COPY, 0};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          case 2: {
            emit_value = (Emit_Value){-1, JUMP_IF_TRUE, 0xFF};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          case 3: {
            frame->stack_data_1 = Emit_Result;
            emit_value = (Emit_Value){-1, POP, 0};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          case 4: {
            Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 1);
            if (Xen_AST_Node_Name_Cmp(expr, "Binary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Unary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Primary") != 0) {
              Xen_DEL_REF(expr);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(expr);
            Xen_DEL_REF(expr);
            frame->passes++;
            break;
          }
          case 5: {
            emit_value = (Emit_Value){frame->stack_data_1, JUMP_IF_TRUE,
                                      code->code->bc_size};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          case 6: {
            emit_value = (Emit_Value){-1, NOP, 0};
            stack[sp++] = Emit;
            frame->passes++;
            break;
          }
          default: {
            --sp;
            break;
          }
          }
        } else {
          switch (frame->passes) {
          case 0: {
            if (Xen_AST_Node_Children_Size(node) != 2) {
              stack[sp++] = Error;
              break;
            }
            Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
            if (Xen_AST_Node_Name_Cmp(expr, "Binary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Unary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Primary") != 0) {
              Xen_DEL_REF(expr);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(expr);
            Xen_DEL_REF(expr);
            frame->passes++;
            break;
          }
          case 1: {
            Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 1);
            if (Xen_AST_Node_Name_Cmp(expr, "Binary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Unary") != 0 &&
                Xen_AST_Node_Name_Cmp(expr, "Primary") != 0) {
              Xen_DEL_REF(expr);
              stack[sp++] = Error;
              break;
            }
            stack[sp++] = FRAME1(expr);
            Xen_DEL_REF(expr);
            frame->passes++;
            break;
          }
          case 2:
            if (Xen_AST_Node_Value_Cmp(node, "**") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_POW};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "*") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_MUL};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "/") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_DIV};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "%") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_MOD};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_ADD};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_SUB};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "<") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_LT};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "<=") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_LE};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "==") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_EQ};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, ">") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_GT};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, ">=") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_GE};
              stack[sp++] = Emit;
              frame->passes++;
            } else if (Xen_AST_Node_Value_Cmp(node, "!=") == 0) {
              emit_value = (Emit_Value){-1, BINARYOP, Xen_OPR_NE};
              stack[sp++] = Emit;
              frame->passes++;
            } else {
              stack[sp++] = Error;
            }
            break;
          default:
            --sp;
            break;
          }
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "Assignment") == 0) {
        switch (frame->passes) {
        case 0: {
          if (Xen_AST_Node_Children_Size(node) != 2) {
            stack[sp++] = Error;
            break;
          }
          Xen_Instance* rhs = Xen_AST_Node_Get_Child(node, 1);
          if (Xen_AST_Node_Name_Cmp(rhs, "Expr") != 0) {
            Xen_DEL_REF(rhs);
            stack[sp++] = Error;
            break;
          }
          stack[sp++] = FRAME1(rhs);
          Xen_DEL_REF(rhs);
          frame->passes++;
          break;
        }
        case 1: {
          Xen_Instance* lhs = Xen_AST_Node_Get_Child(node, 0);
          if (Xen_AST_Node_Name_Cmp(lhs, "Expr") != 0) {
            Xen_DEL_REF(lhs);
            stack[sp++] = Error;
            break;
          }
          stack[sp++] = FRAME2(lhs, MODE_EXPR_ASSIGNMENT_LHS);
          Xen_DEL_REF(lhs);
          frame->passes++;
          break;
        }
        default:
          --sp;
          break;
        }
      } else if (Xen_AST_Node_Name_Cmp(node, "IfStatement") == 0) {
        switch (frame->passes) {
        case 0: {
          Xen_Instance* condition = Xen_AST_Node_Get_Child(node, 0);
          if (Xen_AST_Node_Name_Cmp(condition, "Expr") != 0) {
            Xen_DEL_REF(condition);
            stack[sp++] = Error;
            continue;
          }
          stack[sp++] = FRAME1(condition);
          Xen_DEL_REF(condition);
          frame->passes++;
          break;
        }
        default:
          --sp;
          break;
        }
      } else {
  #ifndef NDEBUG
        printf("Compile Error: Invalid Node '%s'\n", Xen_AST_Node_Name(node));
  #endif
        stack[sp++] = Error;
      }
    }
    Xen_DEL_REF(Emit.node);
    Xen_DEL_REF(Error.node);
    return 1;
  */
}

void program_stack_depth(ProgramCode_t* pc) {
  Xen_ssize_t depth = 0;
  Xen_ssize_t effect = 0;
  for (size_t i = 0; i < pc->code->bc_size; i++) {
    bc_Instruct_t inst = pc->code->bc_array[i];
    effect += Instruct_Info_Table[inst.bci_opcode].stack_effect(inst.bci_oparg);
    if (effect > depth)
      depth = effect;
  }
  pc->stack_depth = depth;
}
