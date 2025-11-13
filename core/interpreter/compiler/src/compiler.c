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
#include "xen_alloc.h"
#include "xen_ast.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"

typedef struct {
  ProgramCode_t* code;
  uint8_t mode;
} Compiler;

CALLABLE_ptr compiler(const char* text_code, uint8_t mode) {
  if (!text_code) {
    return NULL;
  }
#ifndef NDEBUG
  printf("== Parsing ==\n");
#endif
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
#ifndef NDEBUG
  printf("== Compiling ==\n");
#endif
  if (!ast_compile(&pc, mode, ast_program)) {
#ifndef NDEBUG
    printf("Compiler Error\n");
#endif
    Xen_DEL_REF(ast_program);
    vm_consts_free(pc.consts);
    bc_free(pc.code);
    return 0;
  }
  Xen_DEL_REF(ast_program);
#ifndef NDEBUG
  bc_print(pc);
#endif
  Xen_VM_Ctx_Clear(vm->root_context);
  program_stack_depth(&pc);
  CALLABLE_ptr code = callable_new_code(pc);
  if (!code) {
    vm_consts_free(pc.consts);
    bc_free(pc.code);
    return 0;
  }
  return code;
}

#define COMPILE_MODE c.mode
#define OFFSET c.code->code->bc_size

#define emit(offset, opcode, oparg) bc_emit(c.code->code, offset, opcode, oparg)
#define co_push_name(name) vm_consts_push_name(c.code->consts, name)
#define co_push_instance(inst) vm_consts_push_instance(c.code->consts, inst)

static int compile_program(Compiler, Xen_Instance*);

static int compile_statement_list(Compiler, Xen_Instance*);
static int compile_statement(Compiler, Xen_Instance*);

static int compile_expr(Compiler, Xen_Instance*);
static int compile_expr_primary(Compiler, Xen_Instance*);
static int compile_expr_primary_string(Compiler, Xen_Instance*);
static int compile_expr_primary_number(Compiler, Xen_Instance*);
static int compile_expr_primary_literal(Compiler, Xen_Instance*);
static int compile_expr_primary_property(Compiler, Xen_Instance*);
static int compile_expr_primary_parent(Compiler, Xen_Instance*);
static int compile_expr_primary_suffix(Compiler, Xen_Instance*);
static int compile_expr_primary_suffix_call(Compiler, Xen_Instance*);
static Xen_Instance*
compile_expr_primary_suffix_call_arg_assignment(Compiler, Xen_Instance*);
static int compile_expr_primary_suffix_index(Compiler, Xen_Instance*);
static int compile_expr_primary_suffix_attr(Compiler, Xen_Instance*);
static int compile_expr_unary(Compiler, Xen_Instance*);
static int compile_expr_binary(Compiler, Xen_Instance*);
static int compile_expr_list(Compiler, Xen_Instance*);

static int compile_expr_statement(Compiler, Xen_Instance*);

static int compile_assignment(Compiler, Xen_Instance*);
static int compile_assignment_expr(Compiler, Xen_Instance*);
static int compile_assignment_expr_primary(Compiler, Xen_Instance*);
static int compile_assignment_expr_primary_literal(Compiler, Xen_Instance*);
static int compile_assignment_expr_primary_property(Compiler, Xen_Instance*);
static int compile_assignment_expr_primary_parent(Compiler, Xen_Instance*);
static int compile_assignment_expr_primary_suffix(Compiler, Xen_Instance*);
static int compile_assignment_expr_primary_suffix_index(Compiler,
                                                        Xen_Instance*);
static int compile_assignment_expr_primary_suffix_attr(Compiler, Xen_Instance*);

static int compile_block(Compiler, Xen_Instance*);

static int compile_if_statement(Compiler, Xen_Instance*);

static int compile_while_statement(Compiler, Xen_Instance*);

int compile_program(Compiler c, Xen_Instance* node) {
  Xen_Instance* stmt_list = Xen_AST_Node_Get_Child(node, 0);
  if (!stmt_list) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(stmt_list, "StatementList") == 0) {
    if (!compile_statement_list(c, stmt_list)) {
      Xen_DEL_REF(stmt_list);
      return 0;
    }
  } else {
    Xen_DEL_REF(stmt_list);
    return 0;
  }
  Xen_DEL_REF(stmt_list);
  if (emit(-1, NOP, 0) == -1) {
    return 0;
  }
  return 1;
}

int compile_statement_list(Compiler c, Xen_Instance* node) {
  for (Xen_size_t idx = 0; idx < Xen_AST_Node_Children_Size(node); idx++) {
    Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, idx);
    if (!compile_statement(c, stmt)) {
      Xen_DEL_REF(stmt);
      return 0;
    }
    Xen_DEL_REF(stmt);
  }
  return 1;
}

int compile_statement(Compiler c, Xen_Instance* node) {
  Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, 0);
  if (!stmt) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(stmt, "Expr") == 0) {
    if (!compile_expr_statement(c, stmt)) {
      Xen_DEL_REF(stmt);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(stmt, "Assignment") == 0) {
    if (!compile_assignment(c, stmt)) {
      Xen_DEL_REF(stmt);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(stmt, "IfStatement") == 0) {
    if (!compile_if_statement(c, stmt)) {
      Xen_DEL_REF(stmt);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(stmt, "WhileStatement") == 0) {
    if (!compile_while_statement(c, stmt)) {
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

int compile_expr(Compiler c, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
    if (!compile_expr_primary(c, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "Unary") == 0) {
    if (!compile_expr_unary(c, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "Binary") == 0) {
    if (!compile_expr_binary(c, expr)) {
      Xen_DEL_REF(expr);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "List") == 0) {
    if (!compile_expr_list(c, expr)) {
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

int compile_expr_primary(Compiler c, Xen_Instance* node) {
  Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
  if (!primary) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(primary, "String") == 0) {
    if (!compile_expr_primary_string(c, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Number") == 0) {
    if (!compile_expr_primary_number(c, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
    if (!compile_expr_primary_literal(c, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
    if (!compile_expr_primary_property(c, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
    if (!compile_expr_primary_parent(c, primary)) {
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
      if (!compile_expr_primary_suffix(c, suffix)) {
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

int compile_expr_primary_string(Compiler c, Xen_Instance* node) {
  Xen_Instance* string = Xen_String_From_CString(Xen_AST_Node_Value(node));
  if (!string) {
    return 0;
  }
  Xen_ssize_t co_idx = co_push_instance(string);
  if (co_idx == -1) {
    Xen_DEL_REF(string);
    return 0;
  }
  Xen_DEL_REF(string);
  if (emit(-1, PUSH, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_number(Compiler c, Xen_Instance* node) {
  Xen_Instance* number = Xen_Number_From_CString(Xen_AST_Node_Value(node), 0);
  if (!number) {
    return 0;
  }
  Xen_ssize_t co_idx = co_push_instance(number);
  if (co_idx == -1) {
    Xen_DEL_REF(number);
    return 0;
  }
  Xen_DEL_REF(number);
  if (emit(-1, PUSH, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_literal(Compiler c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (emit(-1, LOAD, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_property(Compiler c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (emit(-1, LOAD_PROP, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_parent(Compiler c, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Expr") == 0) {
    if (!compile_expr(c, expr)) {
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

int compile_expr_primary_suffix(Compiler c, Xen_Instance* node) {
  Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 0);
  if (!suffix) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(suffix, "Call") == 0) {
    if (!compile_expr_primary_suffix_call(c, suffix)) {
      Xen_DEL_REF(suffix);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Index") == 0) {
    if (!compile_expr_primary_suffix_index(c, suffix)) {
      Xen_DEL_REF(suffix);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
    if (!compile_expr_primary_suffix_attr(c, suffix)) {
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
      if (!compile_expr_primary_suffix(c, suffix2)) {
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

int compile_expr_primary_suffix_call(Compiler c, Xen_Instance* node) {
  Xen_size_t idx = 0;
  for (; idx < Xen_AST_Node_Children_Size(node); idx++) {
    Xen_Instance* arg = Xen_AST_Node_Get_Child(node, idx);
    if (Xen_AST_Node_Name_Cmp(arg, "Expr") == 0) {
      if (!compile_expr(c, arg)) {
        Xen_DEL_REF(arg);
        return 0;
      }
    } else {
      Xen_DEL_REF(arg);
      break;
    }
    Xen_DEL_REF(arg);
  }
  Xen_size_t kw_count = Xen_AST_Node_Children_Size(node) - idx;
  if (kw_count != 0) {
    Xen_Instance** kw_array = Xen_Alloc(kw_count * sizeof(Xen_Instance*));
    if (!kw_array) {
      return 0;
    }
    for (Xen_size_t i = 0; idx < Xen_AST_Node_Children_Size(node); idx++, i++) {
      Xen_Instance* arg = Xen_AST_Node_Get_Child(node, idx);
      Xen_Instance* kw =
          compile_expr_primary_suffix_call_arg_assignment(c, arg);
      if (!kw) {
        Xen_DEL_REF(arg);
        for (Xen_size_t y = 0; y < i; y++) {
          Xen_DEL_REF(kw_array[y]);
        }
        Xen_Dealloc(kw_array);
        return 0;
      }
      kw_array[i] = kw;
      Xen_DEL_REF(arg);
    }
    Xen_Instance* kw_tuple = Xen_Tuple_From_Array(kw_count, kw_array);
    if (!kw_tuple) {
      for (Xen_size_t i = 0; i < kw_count; i++) {
        Xen_DEL_REF(kw_array[i]);
      }
      Xen_Dealloc(kw_array);
      return 0;
    }
    for (Xen_size_t i = 0; i < kw_count; i++) {
      Xen_DEL_REF(kw_array[i]);
    }
    Xen_Dealloc(kw_array);
    Xen_ssize_t co_idx = co_push_instance(kw_tuple);
    if (co_idx == -1) {
      Xen_DEL_REF(kw_tuple);
      return 0;
    }
    Xen_DEL_REF(kw_tuple);
    if (emit(-1, PUSH, (uint8_t)co_idx) == -1) {
      return 0;
    }
    if (emit(-1, CALL_KW, (uint8_t)Xen_AST_Node_Children_Size(node)) == -1) {
      return 0;
    }
  } else {
    if (emit(-1, CALL, (uint8_t)Xen_AST_Node_Children_Size(node)) == -1) {
      return 0;
    }
  }
  return 1;
}

Xen_Instance*
compile_expr_primary_suffix_call_arg_assignment(Compiler c,
                                                Xen_Instance* node) {
  if (Xen_AST_Node_Name_Cmp(node, "Assignment") != 0 ||
      Xen_AST_Node_Children_Size(node) != 2) {
    return NULL;
  }
  Xen_Instance* rhs = Xen_AST_Node_Get_Child(node, 1);
  if (Xen_AST_Node_Name_Cmp(rhs, "Expr") == 0) {
    if (!compile_expr(c, rhs)) {
      Xen_DEL_REF(rhs);
      return NULL;
    }
  } else {
    Xen_DEL_REF(rhs);
    return NULL;
  }
  Xen_DEL_REF(rhs);
  Xen_Instance* lhs_expr = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_expr, "Expr") != 0 ||
      Xen_AST_Node_Children_Size(lhs_expr) != 1) {
    Xen_DEL_REF(lhs_expr);
    return NULL;
  }
  Xen_Instance* lhs_primary = Xen_AST_Node_Get_Child(lhs_expr, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_primary, "Primary") != 0 ||
      Xen_AST_Node_Children_Size(lhs_primary) != 1) {
    Xen_DEL_REF(lhs_primary);
    Xen_DEL_REF(lhs_expr);
    return NULL;
  }
  Xen_Instance* lhs_literal = Xen_AST_Node_Get_Child(lhs_primary, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_literal, "Literal") != 0) {
    Xen_DEL_REF(lhs_literal);
    Xen_DEL_REF(lhs_primary);
    Xen_DEL_REF(lhs_expr);
    return NULL;
  }
  Xen_Instance* name = Xen_String_From_CString(Xen_AST_Node_Value(lhs_literal));
  if (!name) {
    Xen_DEL_REF(lhs_literal);
    Xen_DEL_REF(lhs_primary);
    Xen_DEL_REF(lhs_expr);
    return NULL;
  }
  Xen_DEL_REF(lhs_literal);
  Xen_DEL_REF(lhs_primary);
  Xen_DEL_REF(lhs_expr);
  return name;
}

int compile_expr_primary_suffix_index(Compiler c, Xen_Instance* node) {
  Xen_Instance* index = Xen_AST_Node_Get_Child(node, 0);
  if (!index) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(index, "Expr") == 0) {
    if (!compile_expr(c, index)) {
      Xen_DEL_REF(index);
      return 0;
    }
  } else {
    Xen_DEL_REF(index);
    return 0;
  }
  Xen_DEL_REF(index);
  if (emit(-1, LOAD_INDEX, 0) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_suffix_attr(Compiler c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (emit(-1, LOAD_ATTR, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_unary(Compiler c, Xen_Instance* node) {
  Xen_Instance* val = Xen_AST_Node_Get_Child(node, 0);
  if (!val) {
    return 0;
  }
  if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
    if (Xen_AST_Node_Name_Cmp(val, "Primary") == 0) {
      if (!compile_expr_primary(c, val)) {
        Xen_DEL_REF(val);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(val, "Unary") == 0) {
      if (!compile_expr_unary(c, val)) {
        Xen_DEL_REF(val);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(val, "Binary") == 0) {
      if (!compile_expr_binary(c, val)) {
        Xen_DEL_REF(val);
        return 0;
      }
    } else {
      Xen_DEL_REF(val);
      return 0;
    }
  } else {
    if (Xen_AST_Node_Name_Cmp(val, "Primary") == 0) {
      if (!compile_expr_primary(c, val)) {
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
    if (emit(-1, UNARY_POSITIVE, 0) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
    if (emit(-1, UNARY_NEGATIVE, 0) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
    if (emit(-1, UNARY_NOT, 0) == -1) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_expr_binary(Compiler c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) != 2) {
    return 0;
  }
  if (Xen_AST_Node_Value_Cmp(node, "and") == 0) {
    Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
      if (!compile_expr_primary(c, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
      if (!compile_expr_unary(c, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
      if (!compile_expr_binary(c, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr1);
      return 0;
    }
    Xen_DEL_REF(expr1);
    if (emit(-1, COPY, 0) == -1) {
      return 0;
    }
    Xen_ssize_t jump_offset = emit(-1, JUMP_IF_FALSE, 0xff);
    if (jump_offset < 0) {
      return 0;
    }
    if (emit(-1, POP, 0) == -1) {
      return 0;
    }
    Xen_Instance* expr2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(expr2, "Primary") == 0) {
      if (!compile_expr_primary(c, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
      if (!compile_expr_unary(c, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
      if (!compile_expr_binary(c, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr2);
      return 0;
    }
    Xen_DEL_REF(expr2);
    emit(jump_offset, JUMP_IF_FALSE, OFFSET);
    return 1;
  }
  if (Xen_AST_Node_Value_Cmp(node, "or") == 0) {
    Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
      if (!compile_expr_primary(c, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
      if (!compile_expr_unary(c, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
      if (!compile_expr_binary(c, expr1)) {
        Xen_DEL_REF(expr1);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr1);
      return 0;
    }
    Xen_DEL_REF(expr1);
    if (emit(-1, COPY, 0) == -1) {
      return 0;
    }
    Xen_ssize_t jump_offset = emit(-1, JUMP_IF_TRUE, 0xff);
    if (jump_offset < 0) {
      return 0;
    }
    if (emit(-1, POP, 0) == -1) {
      return 0;
    }
    Xen_Instance* expr2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(expr2, "Primary") == 0) {
      if (!compile_expr_primary(c, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
      if (!compile_expr_unary(c, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
      if (!compile_expr_binary(c, expr2)) {
        Xen_DEL_REF(expr2);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr2);
      return 0;
    }
    Xen_DEL_REF(expr2);
    emit(jump_offset, JUMP_IF_TRUE, OFFSET);
    return 1;
  }
  Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
    if (!compile_expr_primary(c, expr1)) {
      Xen_DEL_REF(expr1);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
    if (!compile_expr_unary(c, expr1)) {
      Xen_DEL_REF(expr1);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
    if (!compile_expr_binary(c, expr1)) {
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
    if (!compile_expr_primary(c, expr2)) {
      Xen_DEL_REF(expr2);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
    if (!compile_expr_unary(c, expr2)) {
      Xen_DEL_REF(expr2);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
    if (!compile_expr_binary(c, expr2)) {
      Xen_DEL_REF(expr2);
      return 0;
    }
  } else {
    Xen_DEL_REF(expr2);
    return 0;
  }
  Xen_DEL_REF(expr2);
  if (Xen_AST_Node_Value_Cmp(node, "**") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_POW) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "*") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_MUL) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "/") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_DIV) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "%") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_MOD) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_ADD) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_SUB) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "<") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_LT) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "<=") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_LE) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, ">") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_GT) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, ">=") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_GE) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "==") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_EQ) == -1) {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "!=") == 0) {
    if (emit(-1, BINARYOP, (uint8_t)Xen_OPR_NE) == -1) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_expr_list(Compiler c, Xen_Instance* node) {
  for (Xen_size_t idx = 0; idx < Xen_AST_Node_Children_Size(node); idx++) {
    Xen_Instance* expr = Xen_AST_Node_Get_Child(node, idx);
    if (Xen_AST_Node_Name_Cmp(expr, "Binary") == 0) {
      if (!compile_expr_binary(c, expr)) {
        Xen_DEL_REF(expr);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr, "Unary") == 0) {
      if (!compile_expr_unary(c, expr)) {
        Xen_DEL_REF(expr);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
      if (!compile_expr_primary(c, expr)) {
        Xen_DEL_REF(expr);
        return 0;
      }
    } else {
      Xen_DEL_REF(expr);
      return 0;
    }
    Xen_DEL_REF(expr);
  }
  if (emit(-1, MAKE_TUPLE, Xen_AST_Node_Children_Size(node)) == -1) {
    return 0;
  }
  return 1;
}

int compile_expr_statement(Compiler c, Xen_Instance* node) {
  if (!compile_expr(c, node)) {
    return 0;
  }
  if (COMPILE_MODE == Xen_COMPILE_REPL) {
    if (emit(-1, PRINT_TOP, 0) == -1) {
      return 0;
    }
  }
  if (emit(-1, POP, 0) == -1) {
    return 0;
  }
  return 1;
}

int compile_assignment(Compiler c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) != 2) {
    return 0;
  }
  Xen_Instance* rhs = Xen_AST_Node_Get_Child(node, 1);
  if (Xen_AST_Node_Name_Cmp(rhs, "Expr") != 0) {
    Xen_DEL_REF(rhs);
    return 0;
  }
  if (!compile_expr(c, rhs)) {
    Xen_DEL_REF(rhs);
    return 0;
  }
  Xen_DEL_REF(rhs);
  Xen_Instance* lhs = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(lhs, "Expr") != 0) {
    Xen_DEL_REF(lhs);
    return 0;
  }
  if (!compile_assignment_expr(c, lhs)) {
    Xen_DEL_REF(lhs);
    return 0;
  }
  Xen_DEL_REF(lhs);
  return 1;
}

int compile_assignment_expr(Compiler c, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
    if (!compile_assignment_expr_primary(c, expr)) {
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

int compile_assignment_expr_primary(Compiler c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(primary, "String") == 0) {
      if (!compile_expr_primary_string(c, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Number") == 0) {
      if (!compile_expr_primary_number(c, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
      if (!compile_expr_primary_literal(c, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
      if (!compile_expr_primary_property(c, primary)) {
        Xen_DEL_REF(primary);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
      if (!compile_expr_primary_parent(c, primary)) {
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
      if (!compile_assignment_expr_primary_suffix(c, suffix)) {
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
    if (!compile_assignment_expr_primary_literal(c, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
    if (!compile_assignment_expr_primary_property(c, primary)) {
      Xen_DEL_REF(primary);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
    if (!compile_assignment_expr_primary_parent(c, primary)) {
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

int compile_assignment_expr_primary_literal(Compiler c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (emit(-1, STORE, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_property(Compiler c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (emit(-1, STORE_PROP, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_parent(Compiler c, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Expr") == 0) {
    if (!compile_assignment_expr(c, expr)) {
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

int compile_assignment_expr_primary_suffix(Compiler c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(suffix, "Call") == 0) {
      if (!compile_expr_primary_suffix_call(c, suffix)) {
        Xen_DEL_REF(suffix);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(suffix, "Index") == 0) {
      if (!compile_expr_primary_suffix_index(c, suffix)) {
        Xen_DEL_REF(suffix);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
      if (!compile_expr_primary_suffix_attr(c, suffix)) {
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
      if (!compile_assignment_expr_primary_suffix(c, suffix2)) {
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
    if (!compile_assignment_expr_primary_suffix_index(c, suffix)) {
      Xen_DEL_REF(suffix);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
    if (!compile_assignment_expr_primary_suffix_attr(c, suffix)) {
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

int compile_assignment_expr_primary_suffix_index(Compiler c,
                                                 Xen_Instance* node) {
  Xen_Instance* index = Xen_AST_Node_Get_Child(node, 0);
  if (!index) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(index, "Expr") == 0) {
    if (!compile_expr(c, index)) {
      Xen_DEL_REF(index);
      return 0;
    }
  } else {
    Xen_DEL_REF(index);
    return 0;
  }
  Xen_DEL_REF(index);
  if (emit(-1, STORE_INDEX, 0) == -1) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_suffix_attr(Compiler c,
                                                Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (emit(-1, STORE_ATTR, (uint8_t)co_idx) == -1) {
    return 0;
  }
  return 1;
}

int compile_block(Compiler c, Xen_Instance* node) {
  Xen_Instance* block = Xen_AST_Node_Get_Child(node, 0);
  if (!block) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(block, "Statement") == 0) {
    if (!compile_statement(c, block)) {
      Xen_DEL_REF(block);
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(block, "StatementList") == 0) {
    if (!compile_statement_list(c, block)) {
      Xen_DEL_REF(block);
      return 0;
    }
  } else {
    Xen_DEL_REF(block);
    return 0;
  }
  Xen_DEL_REF(block);
  return 1;
}

int compile_if_statement(Compiler c, Xen_Instance* node) {
  Xen_Instance* condition = Xen_AST_Node_Get_Child(node, 0);
  if (!condition) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(condition, "Expr") != 0) {
    Xen_DEL_REF(condition);
    return 0;
  }
  if (!compile_expr(c, condition)) {
    Xen_DEL_REF(condition);
    return 0;
  }
  Xen_DEL_REF(condition);
  Xen_ssize_t jump_if_offset = emit(-1, JUMP_IF_FALSE, 0xFF);
  if (jump_if_offset == -1) {
    return 0;
  }
  Xen_Instance* then = Xen_AST_Node_Get_Child(node, 1);
  if (!then) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(then, "Block") != 0) {
    Xen_DEL_REF(then);
    return 0;
  }
  if (!compile_block(c, then)) {
    Xen_DEL_REF(then);
    return 0;
  }
  Xen_DEL_REF(then);
  if (Xen_AST_Node_Children_Size(node) == 3) {
    Xen_ssize_t jump_end_offset = emit(-1, JUMP, 0xFF);
    if (jump_end_offset == -1) {
      return 0;
    }
    emit(jump_if_offset, JUMP_IF_FALSE, OFFSET);
    Xen_Instance* els = Xen_AST_Node_Get_Child(node, 2);
    if (Xen_AST_Node_Name_Cmp(els, "IfStatement") == 0) {
      if (!compile_if_statement(c, els)) {
        Xen_DEL_REF(els);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(els, "Block") == 0) {
      if (!compile_block(c, els)) {
        Xen_DEL_REF(els);
        return 0;
      }
    } else {
      Xen_DEL_REF(els);
      return 0;
    }
    Xen_DEL_REF(els);
    emit(jump_end_offset, JUMP, OFFSET);
  } else {
    emit(jump_if_offset, JUMP_IF_FALSE, OFFSET);
  }
  return 1;
}

int compile_while_statement(Compiler c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) != 2) {
    return 0;
  }
  Xen_size_t while_init = OFFSET;
  Xen_Instance* condition = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(condition, "Expr") != 0) {
    Xen_DEL_REF(condition);
    return 0;
  }
  if (!compile_expr(c, condition)) {
    Xen_DEL_REF(condition);
    return 0;
  }
  Xen_DEL_REF(condition);
  Xen_ssize_t jump_if_offset = emit(-1, JUMP_IF_FALSE, 0xFF);
  if (jump_if_offset == -1) {
    return 0;
  }
  Xen_Instance* wdo = Xen_AST_Node_Get_Child(node, 1);
  if (!wdo) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(wdo, "Block") != 0) {
    Xen_DEL_REF(wdo);
    return 0;
  }
  if (!compile_block(c, wdo)) {
    Xen_DEL_REF(wdo);
    return 0;
  }
  Xen_DEL_REF(wdo);
  if (emit(-1, JUMP, while_init) == -1) {
    return 0;
  }
  emit(jump_if_offset, JUMP_IF_FALSE, OFFSET);
  return 1;
}

int ast_compile(ProgramCode_t* code, uint8_t mode, Xen_Instance* ast) {
  return compile_program((Compiler){code, mode}, ast);
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
