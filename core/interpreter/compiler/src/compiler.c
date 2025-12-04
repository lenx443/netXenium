#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "attrs.h"
#include "block_list.h"
#include "bytecode.h"
#include "callable.h"
#include "compiler.h"
#include "compiler_ext.h"
#include "gc_header.h"
#include "instance.h"
#include "ir_bytecode.h"
#include "operators.h"
#include "parser.h"
#include "program_code.h"
#include "vm_consts.h"
#include "vm_instructs.h"
#include "xen_alloc.h"
#include "xen_ast.h"
#include "xen_boolean.h"
#include "xen_cstrings.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_typedefs.h"
#include "xen_vector.h"

#define COMPILE_FLAG_FUNCTION_INLINE (1 << 0)

typedef struct {
  block_list_ptr b_list;
  block_node_ptr* b_current;
  struct CompileContext_Stack** cc_stack;
  uint8_t flags;
  uint8_t mode;
} Compiler;

CALLABLE_ptr compiler(Xen_c_string_t file_name, const char* file_content,
                      uint8_t mode) {
  if (!file_name || !file_content) {
    return NULL;
  }
#ifndef NDEBUG
  printf("== Parsing ==\n");
#endif
  Xen_Instance* ast_program =
      Xen_Parser(file_name, file_content, Xen_CString_Len(file_content));
  if (!ast_program) {
    return NULL;
  }
  Xen_IGC_Push(ast_program);
#ifndef NDEBUG
  Xen_AST_Node_Print(ast_program);
#endif
  block_list_ptr blocks = block_list_new();
  if (!blocks) {
    Xen_IGC_Pop();
    return NULL;
  }
  block_node_ptr main_block = block_new();
  if (!main_block) {
    Xen_IGC_Pop();
    block_list_free(blocks);
    return NULL;
  }
  if (!block_list_push_node(blocks, main_block)) {
    Xen_IGC_Pop();
    block_free(main_block);
    block_list_free(blocks);
    return NULL;
  }
#ifndef NDEBUG
  printf("== Compiling ==\n");
#endif
  if (!ast_compile(blocks, &main_block, mode, ast_program)) {
#ifndef NDEBUG
    printf("Compiler Error\n");
#endif
    Xen_IGC_Pop();
    block_list_free(blocks);
    return NULL;
  }
  Xen_IGC_Pop();
  if (!blocks_linealizer(blocks)) {
    block_list_free(blocks);
    return NULL;
  }
#ifndef NDEBUG
  ir_print(blocks);
#endif
  ProgramCode_t pc;
  if (!blocks_compiler(blocks, &pc)) {
    block_list_free(blocks);
    return NULL;
  }
  Xen_GC_Push_Root((Xen_GCHeader*)pc.consts);
  block_list_free(blocks);
#ifndef NDEBUG
//  bc_print(pc);
#endif
  CALLABLE_ptr code = callable_new(pc);
  if (!code) {
    Xen_GC_Pop_Root();
    bc_free(pc.code);
    return NULL;
  }
  Xen_GC_Pop_Root();
  return code;
}

CALLABLE_ptr compiler_ast(Xen_Instance* ast_node, Xen_uint8_t mode) {
  block_list_ptr blocks = block_list_new();
  if (!blocks) {
    return NULL;
  }
  block_node_ptr main_block = block_new();
  if (!main_block) {
    block_list_free(blocks);
    return NULL;
  }
  if (!block_list_push_node(blocks, main_block)) {
    block_free(main_block);
    block_list_free(blocks);
    return NULL;
  }
#ifndef NDEBUG
  printf("== Compiling ==\n");
#endif
  if (!ast_compile(blocks, &main_block, mode, ast_node)) {
#ifndef NDEBUG
    printf("Compiler Error\n");
#endif
    block_list_free(blocks);
    return NULL;
  }
  if (!blocks_linealizer(blocks)) {
    block_list_free(blocks);
    return NULL;
  }
#ifndef NDEBUG
  ir_print(blocks);
#endif
  ProgramCode_t pc;
  if (!blocks_compiler(blocks, &pc)) {
    block_list_free(blocks);
    return NULL;
  }
  Xen_GC_Push_Root((Xen_GCHeader*)pc.consts);
  block_list_free(blocks);
#ifndef NDEBUG
//  bc_print(pc);
#endif
  CALLABLE_ptr code = callable_new(pc);
  if (!code) {
    Xen_GC_Pop_Root();
    bc_free(pc.code);
    return NULL;
  }
  Xen_GC_Pop_Root();
  return code;
}

#define COMPILE_MODE c->mode

#define emit(opcode, oparg, sta)                                               \
  ir_emit((*c->b_current)->instr_array, opcode, oparg, sta)
#define emit_jump(opcode, block, sta)                                          \
  ir_emit_jump((*c->b_current)->instr_array, opcode, block, sta)
#define co_push_name(name) vm_consts_push_name(c->b_list->consts, name)
#define co_push_instance(inst) vm_consts_push_instance(c->b_list->consts, inst)
#define co_push_callable(callable)                                             \
  vm_consts_push_callable(c->b_list->consts, callable)

#define B_PTR block_node_ptr
#define B_NEW() block_new()
#define B_FREE(node) block_free(node)
#define B_LIST_PUSH(block) block_list_push_node(c->b_list, block)
#define B_CURRENT *c->b_current
#define B_SET_CURRENT(block) *c->b_current = block
#define B_MAKE_CURRENT() __B_MAKE_CURRENT(c)

#define CC struct CompileContext
#define CCS struct CompileContext_Stack
#define CCS_ptr struct CompileContext_Stack*
#define CC_TOP (*c->cc_stack)->context
#define CCS_TOP *c->cc_stack
#define CCS_PUSH(lc) compile_context_stack_push(c->cc_stack, lc)
#define CCS_POP compile_context_stack_pop(c->cc_stack)

static inline B_PTR __B_MAKE_CURRENT(Compiler* c) {
  B_PTR node = B_NEW();
  if (!node) {
    return NULL;
  }
  if (!B_LIST_PUSH(node)) {
    B_FREE(node);
    return NULL;
  }
  B_SET_CURRENT(node);
  return node;
}

struct CompileContext {
  B_PTR b_break;
  B_PTR b_continue;
};

struct CompileContext_Stack {
  struct CompileContext context;
  struct CompileContext_Stack* next;
};

static int compile_context_stack_push(struct CompileContext_Stack**,
                                      struct CompileContext);
static void compile_context_stack_pop(struct CompileContext_Stack**);

int compile_context_stack_push(struct CompileContext_Stack** ccs,
                               struct CompileContext cc) {
  assert(ccs != NULL);
  CCS_ptr ccs_new = Xen_Alloc(sizeof(CCS));
  if (!ccs_new) {
    return 0;
  }
  ccs_new->context = cc;
  ccs_new->next = *ccs;
  *ccs = ccs_new;
  return 1;
}

void compile_context_stack_pop(struct CompileContext_Stack** ccs) {
  assert(ccs != NULL && *ccs != NULL);
  CCS_ptr temp = *ccs;
  *ccs = (*ccs)->next;
  Xen_Dealloc(temp);
}

static int compile_program(Compiler*, Xen_Instance*);

static int compile_statement_list(Compiler*, Xen_Instance*);
static int compile_statement(Compiler*, Xen_Instance*);

static int compile_expr_statement(Compiler*, Xen_Instance*);
static Xen_Instance* compile_expr_constant(int*, Xen_Instance*);
static int compile_expr(Compiler*, Xen_Instance*);
static int compile_expr_primary(Compiler*, Xen_Instance*);
static int compile_expr_primary_string(Compiler*, Xen_Instance*);
static int compile_expr_primary_number(Compiler*, Xen_Instance*);
static int compile_expr_primary_nil(Compiler*, Xen_Instance*);
static int compile_expr_primary_literal(Compiler*, Xen_Instance*);
static int compile_expr_primary_property(Compiler*, Xen_Instance*);
static int compile_expr_primary_parent(Compiler*, Xen_Instance*);
static int compile_expr_primary_map(Compiler*, Xen_Instance*);
static int compile_expr_primary_map_element(Compiler*, Xen_Instance*);
static int compile_expr_primary_suffix(Compiler*, Xen_Instance*);
static int compile_expr_primary_suffix_call(Compiler*, Xen_Instance*);
static Xen_Instance*
compile_expr_primary_suffix_call_arg_assignment(Compiler*, Xen_Instance*);
static int compile_expr_primary_suffix_index(Compiler*, Xen_Instance*);
static int compile_expr_primary_suffix_attr(Compiler*, Xen_Instance*);
static int compile_expr_unary(Compiler*, Xen_Instance*);
static int compile_expr_binary(Compiler*, Xen_Instance*);
static int compile_expr_function(Compiler*, Xen_Instance*);
static Xen_Instance* compile_expr_function_arg_assigment(Compiler*,
                                                         Xen_Instance*);
static int compile_expr_list(Compiler*, Xen_Instance*);

static int compile_assignment(Compiler*, Xen_Instance*);
static int compile_assignment_expr(Compiler*, Xen_Instance*);
static int compile_assignment_expr_primary(Compiler*, Xen_Instance*);
static int compile_assignment_expr_primary_literal(Compiler*, Xen_Instance*);
static int compile_assignment_expr_primary_property(Compiler*, Xen_Instance*);
static int compile_assignment_expr_primary_parent(Compiler*, Xen_Instance*);
static int compile_assignment_expr_primary_suffix(Compiler*, Xen_Instance*);
static int compile_assignment_expr_primary_suffix_index(Compiler*,
                                                        Xen_Instance*);
static int compile_assignment_expr_primary_suffix_attr(Compiler*,
                                                       Xen_Instance*);
static int compile_assignment_expr_list(Compiler*, Xen_Instance*);

static int compile_block(Compiler*, Xen_Instance*);

static int compile_if_statement(Compiler*, Xen_Instance*);

static int compile_while_statement(Compiler*, Xen_Instance*);

static int compile_for_statement(Compiler*, Xen_Instance*);

static int compile_flow_statement(Compiler*, Xen_Instance*);

static int compile_return_statement(Compiler*, Xen_Instance*);

static int compile_implement_statement(Compiler*, Xen_Instance*);

int compile_program(Compiler* c, Xen_Instance* node) {
  if (COMPILE_MODE == Xen_COMPILE_FUNCTION) {
    if (Xen_AST_Node_Name_Cmp(node, "StatementList") == 0) {
      if (!compile_statement_list(c, node)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(node, "Statement") == 0) {
      c->flags |= COMPILE_FLAG_FUNCTION_INLINE;
      if (!compile_statement(c, node)) {
        return 0;
      }
    } else {
      return 0;
    }
    if ((c->flags & COMPILE_FLAG_FUNCTION_INLINE) == 0) {
      if (!emit(RETURN, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
    }
  } else if (COMPILE_MODE == Xen_COMPILE_IMPLEMENT) {
    if (Xen_AST_Node_Name_Cmp(node, "Block") == 0) {
      if (!compile_block(c, node)) {
        return 0;
      }
    } else {
      return 0;
    }
    if (!emit(RETURN_BUILD_IMPLEMENT, 0, Xen_AST_Node_STA(node))) {
      return 0;
    }
  } else {
    Xen_Instance* stmt_list = Xen_AST_Node_Get_Child(node, 0);
    if (!stmt_list) {
      return 0;
    }
    if (Xen_AST_Node_Name_Cmp(stmt_list, "StatementList") == 0) {
      if (!compile_statement_list(c, stmt_list)) {
        return 0;
      }
    } else {
      return 0;
    }
    if (!emit(RETURN, 0, Xen_AST_Node_STA(node))) {
      return 0;
    }
  }
  return 1;
}

int compile_statement_list(Compiler* c, Xen_Instance* node) {
  for (Xen_size_t idx = 0; idx < Xen_AST_Node_Children_Size(node); idx++) {
    Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, idx);
    if (!compile_statement(c, stmt)) {
      return 0;
    }
  }
  return 1;
}

int compile_statement(Compiler* c, Xen_Instance* node) {
  Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, 0);
  if (!stmt) {
    return 0;
  }
  if (COMPILE_MODE == Xen_COMPILE_IMPLEMENT) {
    if (Xen_AST_Node_Name_Cmp(stmt, "Assignment") == 0) {
      if (!compile_assignment(c, stmt)) {
        return 0;
      }
    } else {
      return 0;
    }
  } else {
    if (Xen_AST_Node_Name_Cmp(stmt, "Expr") == 0) {
      if (!compile_expr_statement(c, stmt)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(stmt, "Assignment") == 0) {
      if (!compile_assignment(c, stmt)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(stmt, "IfStatement") == 0) {
      if (!compile_if_statement(c, stmt)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(stmt, "WhileStatement") == 0) {
      if (!compile_while_statement(c, stmt)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(stmt, "ForStatement") == 0) {
      if (!compile_for_statement(c, stmt)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(stmt, "FlowStatement") == 0) {
      if (!compile_flow_statement(c, stmt)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(stmt, "ReturnStatement") == 0) {
      if (!compile_return_statement(c, stmt)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(stmt, "ImplementStatement") == 0) {
      if (!compile_implement_statement(c, stmt)) {
        return 0;
      }
    } else {
      return 0;
    }
  }
  return 1;
}

int compile_expr_statement(Compiler* c, Xen_Instance* node) {
  if (!compile_expr(c, node)) {
    return 0;
  }
  if (COMPILE_MODE == Xen_COMPILE_REPL) {
    if (!emit(PRINT_TOP, 0, Xen_AST_Node_STA(node))) {
      return 0;
    }
  }
  if (c->flags & COMPILE_FLAG_FUNCTION_INLINE) {
    if (!emit(RETURN_TOP, 0, Xen_AST_Node_STA(node))) {
      return 0;
    }
  } else {
    if (!emit(POP, 1, Xen_AST_Node_STA(node))) {
      return 0;
    }
  }
  return 1;
}

Xen_Instance* compile_expr_constant(int* error, Xen_Instance* node) {
  *error = 1;
  if (Xen_AST_Node_Name_Cmp(node, "Expr") == 0) {
    Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
    if (!expr) {
      *error = -1;
      return NULL;
    }
    if (Xen_AST_Node_Name_Cmp(expr, "Primary") != 0 &&
        Xen_AST_Node_Name_Cmp(expr, "Unary") != 0 &&
        Xen_AST_Node_Name_Cmp(expr, "Binary") != 0 &&
        Xen_AST_Node_Name_Cmp(expr, "List") != 0 &&
        Xen_AST_Node_Name_Cmp(expr, "Nil") != 0) {
      *error = 0;
      return NULL;
    }
    Xen_Instance* result = compile_expr_constant(error, expr);
    if (!result) {
      return NULL;
    }
    return result;
  } else if (Xen_AST_Node_Name_Cmp(node, "Number") == 0) {
    Xen_Instance* number = Xen_Number_From_CString(Xen_AST_Node_Value(node), 0);
    if (!number) {
      *error = -1;
      return NULL;
    }
    return number;
  } else if (Xen_AST_Node_Name_Cmp(node, "String") == 0) {
    Xen_Instance* string = Xen_String_From_CString(Xen_AST_Node_Value(node));
    if (!string) {
      *error = -1;
      return NULL;
    }
    return string;
  } else if (Xen_AST_Node_Name_Cmp(node, "Parent") == 0) {
    Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
    if (!expr) {
      *error = -1;
      return NULL;
    }
    if (Xen_AST_Node_Name_Cmp(expr, "Expr") != 0) {
      *error = -1;
      return NULL;
    }
    Xen_Instance* result = compile_expr_constant(error, expr);
    if (!result) {
      return NULL;
    }
    return result;
  } else if (Xen_AST_Node_Name_Cmp(node, "Primary") == 0) {
    Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
    if (!primary) {
      *error = -1;
      return NULL;
    }
    if (Xen_AST_Node_Name_Cmp(primary, "Number") != 0 &&
        Xen_AST_Node_Name_Cmp(primary, "String") != 0 &&
        Xen_AST_Node_Name_Cmp(primary, "Parent") != 0) {
      *error = 0;
      return NULL;
    }
    Xen_Instance* result = compile_expr_constant(error, primary);
    if (!result) {
      return NULL;
    }
    return result;
  } else if (Xen_AST_Node_Name_Cmp(node, "Unary") == 0) {
    Xen_Instance* unary = Xen_AST_Node_Get_Child(node, 0);
    if (!unary) {
      *error = -1;
      return NULL;
    }
    if (Xen_AST_Node_Name_Cmp(unary, "Primary") != 0 &&
        Xen_AST_Node_Name_Cmp(unary, "Unary") != 0 &&
        Xen_AST_Node_Name_Cmp(unary, "Binary") != 0) {
      *error = -1;
      return NULL;
    }
    Xen_Instance* primary = compile_expr_constant(error, unary);
    if (!primary) {
      return NULL;
    }
    Xen_IGC_Push(primary);
    if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
      Xen_Instance* result =
          Xen_Method_Attr_Str_Call(primary, "__positive", nil, nil);
      if (!result) {
        Xen_IGC_Pop();
        *error = -1;
        return NULL;
      }
      Xen_IGC_Pop();
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
      Xen_Instance* result =
          Xen_Method_Attr_Str_Call(primary, "__negative", nil, nil);
      if (!result) {
        Xen_IGC_Pop();
        *error = -1;
        return NULL;
      }
      Xen_IGC_Pop();
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
      Xen_Instance* result =
          Xen_Method_Attr_Str_Call(primary, "__not", nil, nil);
      if (!result) {
        Xen_IGC_Pop();
        *error = -1;
        return NULL;
      }
      Xen_IGC_Pop();
      return result;
    } else {
      Xen_IGC_Pop();
      *error = 0;
      return NULL;
    }
  } else if (Xen_AST_Node_Name_Cmp(node, "Binary") == 0) {
    Xen_Instance* lhs = Xen_AST_Node_Get_Child(node, 0);
    if (!lhs) {
      *error = -1;
      return NULL;
    }
    Xen_Instance* rhs = Xen_AST_Node_Get_Child(node, 1);
    if (!rhs) {
      *error = -1;
      return NULL;
    }
    if (Xen_AST_Node_Name_Cmp(lhs, "Primary") != 0 &&
        Xen_AST_Node_Name_Cmp(lhs, "Unary") != 0 &&
        Xen_AST_Node_Name_Cmp(lhs, "Binary") != 0) {
      *error = -1;
      return NULL;
    }
    if (Xen_AST_Node_Name_Cmp(rhs, "Primary") != 0 &&
        Xen_AST_Node_Name_Cmp(rhs, "Unary") != 0 &&
        Xen_AST_Node_Name_Cmp(rhs, "Binary") != 0) {
      *error = -1;
      return NULL;
    }
    Xen_size_t roots = 0;
    Xen_Instance* lhs_expr = compile_expr_constant(error, lhs);
    if (!lhs_expr) {
      return NULL;
    }
    Xen_IGC_XPUSH(lhs_expr, roots);
    Xen_Instance* rhs_expr = compile_expr_constant(error, rhs);
    if (!rhs_expr) {
      Xen_IGC_XPOP(roots);
      return NULL;
    }
    Xen_IGC_XPUSH(rhs_expr, roots);
    if (Xen_AST_Node_Value_Cmp(node, "**") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_POW);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "*") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_MUL);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "/") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_DIV);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "%") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_MOD);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_ADD);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_SUB);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "<") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_LT);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "<=") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_LE);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, ">") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_GT);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, ">=") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_GE);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "==") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_EQ);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "!=") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_NE);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else if (Xen_AST_Node_Value_Cmp(node, "and") == 0) {
      Xen_Instance* lhs_bool = Xen_Attr_Boolean(lhs_expr);
      if (!lhs_bool) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      if (lhs_bool == Xen_True) {
        Xen_IGC_XPOP(roots);
        return rhs_expr;
      }
      Xen_IGC_XPOP(roots);
      return lhs_expr;
    } else if (Xen_AST_Node_Value_Cmp(node, "or") == 0) {
      Xen_Instance* lhs_bool = Xen_Attr_Boolean(lhs_expr);
      if (!lhs_bool) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      if (lhs_bool == Xen_False) {
        Xen_IGC_XPOP(roots);
        return rhs_expr;
      }
      Xen_IGC_XPOP(roots);
      return lhs_expr;
    } else if (Xen_AST_Node_Value_Cmp(node, "has") == 0) {
      Xen_Instance* result =
          Xen_Operator_Eval_Pair(lhs_expr, rhs_expr, Xen_OPR_HAS);
      if (!result) {
        Xen_IGC_XPOP(roots);
        *error = -1;
        return NULL;
      }
      Xen_IGC_XPOP(roots);
      return result;
    } else {
      Xen_IGC_XPOP(roots);
      *error = 0;
      return NULL;
    }
  } else if (Xen_AST_Node_Name_Cmp(node, "List") == 0) {
    Xen_size_t count = Xen_AST_Node_Children_Size(node);
    Xen_Instance** values = Xen_Alloc(count * sizeof(Xen_Instance*));
    if (!values) {
      *error = -1;
      return 0;
    }
    Xen_size_t roots = 0;
    for (Xen_size_t idx = 0; idx < count; idx++) {
      Xen_Instance* expr = Xen_AST_Node_Get_Child(node, idx);
      Xen_Instance* value = compile_expr_constant(error, expr);
      if (!value) {
        Xen_IGC_XPOP(roots);
        Xen_Dealloc(values);
        return NULL;
      }
      values[idx] = value;
      Xen_IGC_XPUSH(value, roots);
    }
    Xen_Instance* result = Xen_Tuple_From_Array(count, values);
    if (!result) {
      Xen_IGC_XPOP(roots);
      Xen_Dealloc(values);
      *error = -1;
      return NULL;
    }
    Xen_IGC_XPOP(roots);
    Xen_Dealloc(values);
    return result;
  } else if (Xen_AST_Node_Name_Cmp(node, "Nil") == 0) {
    return nil;
  } else {
    *error = 0;
    return NULL;
  }
}

int compile_expr(Compiler* c, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
    if (!compile_expr_primary(c, expr)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "Unary") == 0) {
    if (!compile_expr_unary(c, expr)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "Binary") == 0) {
    if (!compile_expr_binary(c, expr)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "FunctionExpr") == 0) {
    if (!compile_expr_function(c, expr)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "List") == 0) {
    if (!compile_expr_list(c, expr)) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_expr_primary(Compiler* c, Xen_Instance* node) {
  Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
  if (!primary) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(primary, "String") == 0) {
    if (!compile_expr_primary_string(c, primary)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Number") == 0) {
    if (!compile_expr_primary_number(c, primary)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Nil") == 0) {
    if (!compile_expr_primary_nil(c, primary)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
    if (!compile_expr_primary_literal(c, primary)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
    if (!compile_expr_primary_property(c, primary)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
    if (!compile_expr_primary_parent(c, primary)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(primary, "Map") == 0) {
    if (!compile_expr_primary_map(c, primary)) {
      return 0;
    }
  } else {
    return 0;
  }
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(suffix, "Suffix") == 0) {
      if (!compile_expr_primary_suffix(c, suffix)) {
        return 0;
      }
    } else {
      return 0;
    }
  }
  return 1;
}

int compile_expr_primary_string(Compiler* c, Xen_Instance* node) {
  Xen_Instance* string = Xen_String_From_CString(Xen_AST_Node_Value(node));
  if (!string) {
    return 0;
  }
  Xen_ssize_t co_idx = co_push_instance(string);
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_number(Compiler* c, Xen_Instance* node) {
  Xen_Instance* number = Xen_Number_From_CString(Xen_AST_Node_Value(node), 0);
  if (!number) {
    return 0;
  }
  Xen_ssize_t co_idx = co_push_instance(number);
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_nil(Compiler* c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_instance(nil);
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_literal(Compiler* c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(LOAD, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_property(Compiler* c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(LOAD_PROP, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_parent(Compiler* c, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Expr") == 0) {
    if (!compile_expr(c, expr)) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_expr_primary_map(Compiler* c, Xen_Instance* node) {
  Xen_size_t count = Xen_AST_Node_Children_Size(node);
  for (Xen_size_t i = count; i-- > 0;) {
    Xen_Instance* element = Xen_AST_Node_Get_Child(node, i);
    if (Xen_AST_Node_Name_Cmp(element, "MapElement") != 0) {
      return 0;
    }
    if (!compile_expr_primary_map_element(c, element)) {
      return 0;
    }
  }
  if (!emit(MAKE_MAP, count, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_map_element(Compiler* c, Xen_Instance* node) {
  Xen_Instance* key = Xen_AST_Node_Get_Child(node, 0);
  if (!key) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(key, "Primary") == 0) {
    if (!compile_expr_primary(c, key)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(key, "Literal") == 0) {
    Xen_Instance* literal = Xen_String_From_CString(Xen_AST_Node_Value(key));
    if (!literal) {
      return 0;
    }
    Xen_ssize_t co_idx = co_push_instance(literal);
    if (co_idx == -1) {
      return 0;
    }
    if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
      return 0;
    }
  } else {
    return 0;
  }
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* value = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(value, "Expr") != 0) {
      return 0;
    }
    if (!compile_expr(c, value)) {
      return 0;
    }
  } else {
    Xen_ssize_t co_idx = co_push_instance(nil);
    if (co_idx == -1) {
      return 0;
    }
    if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
      return 0;
    }
  }
  return 1;
}

int compile_expr_primary_suffix(Compiler* c, Xen_Instance* node) {
  Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 0);
  if (!suffix) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(suffix, "Call") == 0) {
    if (!compile_expr_primary_suffix_call(c, suffix)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Index") == 0) {
    if (!compile_expr_primary_suffix_index(c, suffix)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
    if (!compile_expr_primary_suffix_attr(c, suffix)) {
      return 0;
    }
  } else {
    return 0;
  }
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* suffix2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(suffix2, "Suffix") == 0) {
      if (!compile_expr_primary_suffix(c, suffix2)) {
        return 0;
      }
    } else {
      return 0;
    }
  }
  return 1;
}

int compile_expr_primary_suffix_call(Compiler* c, Xen_Instance* node) {
  Xen_size_t idx = 0;
  for (; idx < Xen_AST_Node_Children_Size(node); idx++) {
    Xen_Instance* arg = Xen_AST_Node_Get_Child(node, idx);
    if (Xen_AST_Node_Name_Cmp(arg, "Expr") == 0) {
      if (!compile_expr(c, arg)) {
        return 0;
      }
    } else {
      break;
    }
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
        Xen_Dealloc(kw_array);
        return 0;
      }
      kw_array[i] = kw;
    }
    Xen_Instance* kw_tuple = Xen_Tuple_From_Array(kw_count, kw_array);
    if (!kw_tuple) {
      Xen_Dealloc(kw_array);
      return 0;
    }
    Xen_Dealloc(kw_array);
    Xen_ssize_t co_idx = co_push_instance(kw_tuple);
    if (co_idx == -1) {
      return 0;
    }
    if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
      return 0;
    }
    if (!emit(CALL_KW, Xen_AST_Node_Children_Size(node),
              Xen_AST_Node_STA(node))) {
      return 0;
    }
  } else {
    if (!emit(CALL, Xen_AST_Node_Children_Size(node), Xen_AST_Node_STA(node))) {
      return 0;
    }
  }
  return 1;
}

Xen_Instance*
compile_expr_primary_suffix_call_arg_assignment(Compiler* c,
                                                Xen_Instance* node) {
  if (Xen_AST_Node_Name_Cmp(node, "Assignment") != 0 ||
      Xen_AST_Node_Children_Size(node) != 2) {
    return NULL;
  }
  Xen_Instance* rhs = Xen_AST_Node_Get_Child(node, 1);
  if (Xen_AST_Node_Name_Cmp(rhs, "Expr") == 0) {
    if (!compile_expr(c, rhs)) {
      return NULL;
    }
  } else if (Xen_AST_Node_Name_Cmp(rhs, "FunctionExpr") == 0) {
    if (!compile_expr_function(c, rhs)) {
      return 0;
    }
  } else {
    return NULL;
  }
  Xen_Instance* lhs_expr = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_expr, "Expr") != 0 ||
      Xen_AST_Node_Children_Size(lhs_expr) != 1) {
    return NULL;
  }
  Xen_Instance* lhs_primary = Xen_AST_Node_Get_Child(lhs_expr, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_primary, "Primary") != 0 ||
      Xen_AST_Node_Children_Size(lhs_primary) != 1) {
    return NULL;
  }
  Xen_Instance* lhs_literal = Xen_AST_Node_Get_Child(lhs_primary, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_literal, "Literal") != 0) {
    return NULL;
  }
  Xen_Instance* name = Xen_String_From_CString(Xen_AST_Node_Value(lhs_literal));
  if (!name) {
    return NULL;
  }
  return name;
}

int compile_expr_primary_suffix_index(Compiler* c, Xen_Instance* node) {
  Xen_Instance* index = Xen_AST_Node_Get_Child(node, 0);
  if (!index) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(index, "Expr") == 0) {
    if (!compile_expr(c, index)) {
      return 0;
    }
  } else {
    return 0;
  }
  if (!emit(LOAD_INDEX, 0, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_expr_primary_suffix_attr(Compiler* c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(LOAD_ATTR, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_expr_unary(Compiler* c, Xen_Instance* node) {
  int error;
  Xen_Instance* constant_value = compile_expr_constant(&error, node);
  if (constant_value) {
    Xen_ssize_t co_idx = co_push_instance(constant_value);
    if (co_idx == -1) {
      return 0;
    }
    if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
      return 0;
    }
    return 1;
  } else if (error == 0) {
    Xen_Instance* val = Xen_AST_Node_Get_Child(node, 0);
    if (!val) {
      return 0;
    }
    if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
      if (Xen_AST_Node_Name_Cmp(val, "Primary") == 0) {
        if (!compile_expr_primary(c, val)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(val, "Unary") == 0) {
        if (!compile_expr_unary(c, val)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(val, "Binary") == 0) {
        if (!compile_expr_binary(c, val)) {
          return 0;
        }
      } else {
        return 0;
      }
    } else {
      if (Xen_AST_Node_Name_Cmp(val, "Primary") == 0) {
        if (!compile_expr_primary(c, val)) {
          return 0;
        }
      } else {
        return 0;
      }
    }
    if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
      if (!emit(UNARY_POSITIVE, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
      if (!emit(UNARY_NEGATIVE, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
      if (!emit(UNARY_NOT, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else {
      return 0;
    }
    return 1;
  }
  return 0;
}

int compile_expr_binary(Compiler* c, Xen_Instance* node) {
  int error;
  Xen_Instance* constant_value = compile_expr_constant(&error, node);
  if (constant_value) {
    Xen_ssize_t co_idx = co_push_instance(constant_value);
    if (co_idx == -1) {
      return 0;
    }
    if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
      return 0;
    }
    return 1;
  } else if (error == 0) {
    if (Xen_AST_Node_Children_Size(node) != 2) {
      return 0;
    }
    if (Xen_AST_Node_Value_Cmp(node, "and") == 0) {
      Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
      if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
        if (!compile_expr_primary(c, expr1)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
        if (!compile_expr_unary(c, expr1)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
        if (!compile_expr_binary(c, expr1)) {
          return 0;
        }
      } else {
        return 0;
      }
      if (!emit(COPY, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
      B_PTR end_block = B_NEW();
      if (!end_block) {
        return 0;
      }
      if (!emit_jump(JUMP_IF_FALSE, end_block, Xen_AST_Node_STA(node))) {
        B_FREE(end_block);
        return 0;
      }
      if (!emit(POP, 1, Xen_AST_Node_STA(node))) {
        B_FREE(end_block);
        return 0;
      }
      Xen_Instance* expr2 = Xen_AST_Node_Get_Child(node, 1);
      if (Xen_AST_Node_Name_Cmp(expr2, "Primary") == 0) {
        if (!compile_expr_primary(c, expr2)) {
          B_FREE(end_block);
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
        if (!compile_expr_unary(c, expr2)) {
          B_FREE(end_block);
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
        if (!compile_expr_binary(c, expr2)) {
          B_FREE(end_block);
          return 0;
        }
      } else {
        B_FREE(end_block);
        return 0;
      }
      if (!B_LIST_PUSH(end_block)) {
        B_FREE(end_block);
        return 0;
      }
      B_SET_CURRENT(end_block);
      return 1;
    }
    if (Xen_AST_Node_Value_Cmp(node, "or") == 0) {
      Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
      if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
        if (!compile_expr_primary(c, expr1)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
        if (!compile_expr_unary(c, expr1)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
        if (!compile_expr_binary(c, expr1)) {
          return 0;
        }
      } else {
        return 0;
      }
      if (!emit(COPY, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
      B_PTR end_block = B_NEW();
      if (!end_block) {
        return 0;
      }
      if (!emit_jump(JUMP_IF_TRUE, end_block, Xen_AST_Node_STA(node))) {
        B_FREE(end_block);
        return 0;
      }
      if (!emit(POP, 1, Xen_AST_Node_STA(node))) {
        B_FREE(end_block);
        return 0;
      }
      Xen_Instance* expr2 = Xen_AST_Node_Get_Child(node, 1);
      if (Xen_AST_Node_Name_Cmp(expr2, "Primary") == 0) {
        if (!compile_expr_primary(c, expr2)) {
          B_FREE(end_block);
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
        if (!compile_expr_unary(c, expr2)) {
          B_FREE(end_block);
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
        if (!compile_expr_binary(c, expr2)) {
          B_FREE(end_block);
          return 0;
        }
      } else {
        B_FREE(end_block);
        return 0;
      }
      if (!B_LIST_PUSH(end_block)) {
        B_FREE(end_block);
        return 0;
      }
      B_SET_CURRENT(end_block);
      return 1;
    }
    Xen_Instance* expr1 = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(expr1, "Primary") == 0) {
      if (!compile_expr_primary(c, expr1)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Unary") == 0) {
      if (!compile_expr_unary(c, expr1)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr1, "Binary") == 0) {
      if (!compile_expr_binary(c, expr1)) {
        return 0;
      }
    } else {
      return 0;
    }
    Xen_Instance* expr2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(expr2, "Primary") == 0) {
      if (!compile_expr_primary(c, expr2)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Unary") == 0) {
      if (!compile_expr_unary(c, expr2)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr2, "Binary") == 0) {
      if (!compile_expr_binary(c, expr2)) {
        return 0;
      }
    } else {
      return 0;
    }
    if (Xen_AST_Node_Value_Cmp(node, "**") == 0) {
      if (!emit(BINARYOP, Xen_OPR_POW, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "*") == 0) {
      if (!emit(BINARYOP, Xen_OPR_MUL, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "/") == 0) {
      if (!emit(BINARYOP, Xen_OPR_DIV, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "%") == 0) {
      if (!emit(BINARYOP, Xen_OPR_MOD, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
      if (!emit(BINARYOP, Xen_OPR_ADD, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
      if (!emit(BINARYOP, Xen_OPR_SUB, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "<") == 0) {
      if (!emit(BINARYOP, Xen_OPR_LT, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "<=") == 0) {
      if (!emit(BINARYOP, Xen_OPR_LE, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, ">") == 0) {
      if (!emit(BINARYOP, Xen_OPR_GT, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, ">=") == 0) {
      if (!emit(BINARYOP, Xen_OPR_GE, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "==") == 0) {
      if (!emit(BINARYOP, Xen_OPR_EQ, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "!=") == 0) {
      if (!emit(BINARYOP, Xen_OPR_NE, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "has") == 0) {
      if (!emit(BINARYOP, Xen_OPR_HAS, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else {
      return 0;
    }
    return 1;
  }
  return 0;
}

int compile_expr_function(Compiler* c, Xen_Instance* node) {
  Xen_size_t roots = 0;
  Xen_Instance* args_positionals_requireds_vector = Xen_Vector_New();
  if (!args_positionals_requireds_vector) {
    return 0;
  }
  Xen_IGC_XPUSH(args_positionals_requireds_vector, roots);
  Xen_size_t idx = 0;
  Xen_Instance* args = Xen_AST_Node_Get_Child(node, 0);
  if (!args) {
    Xen_IGC_XPOP(roots);
    return 0;
  }
  for (; idx < Xen_AST_Node_Children_Size(args); idx++) {
    Xen_Instance* arg = Xen_AST_Node_Get_Child(args, idx);
    if (Xen_AST_Node_Name_Cmp(arg, "Expr") == 0) {
      Xen_Instance* arg_primary = Xen_AST_Node_Get_Child(arg, 0);
      if (Xen_AST_Node_Name_Cmp(arg_primary, "Primary") != 0 ||
          Xen_AST_Node_Children_Size(arg_primary) != 1) {
        Xen_IGC_XPOP(roots);
        return 0;
      }
      Xen_Instance* arg_literal = Xen_AST_Node_Get_Child(arg_primary, 0);
      if (Xen_AST_Node_Name_Cmp(arg_literal, "Literal") != 0) {
        Xen_IGC_XPOP(roots);
        return 0;
      }
      Xen_Instance* name =
          Xen_String_From_CString(Xen_AST_Node_Value(arg_literal));
      if (!name) {
        Xen_IGC_XPOP(roots);
        return 0;
      }
      if (!Xen_Vector_Push(args_positionals_requireds_vector, name)) {
        Xen_IGC_XPOP(roots);
        return 0;
      }
    } else {
      break;
    }
  }
  Xen_size_t args_assigment_start = idx;
  for (; idx < Xen_AST_Node_Children_Size(args); idx++) {
    Xen_Instance* arg = Xen_AST_Node_Get_Child(args, idx);
    Xen_Instance* name = compile_expr_function_arg_assigment(c, arg);
    if (!name) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    if (!Xen_Vector_Push(args_positionals_requireds_vector, name)) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
  }
  if (Xen_SIZE(args_positionals_requireds_vector) == 0) {
    Xen_Instance* body = Xen_AST_Node_Get_Child(node, 1);
    if (!body) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    CALLABLE_ptr code = compiler_ast(body, Xen_COMPILE_FUNCTION);
    if (!code) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    Xen_ssize_t co_code_idx = co_push_callable(code);
    if (co_code_idx == -1) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    if (!emit(MAKE_FUNCTION_NARGS, co_code_idx, Xen_AST_Node_STA(node))) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
  } else {
    Xen_Instance* args_positionals_requireds =
        Xen_Tuple_From_Vector(args_positionals_requireds_vector);
    if (!args_positionals_requireds) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    if (!emit(MAKE_TUPLE,
              Xen_AST_Node_Children_Size(args) - args_assigment_start,
              Xen_AST_Node_STA(node))) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    Xen_ssize_t co_args_idx = co_push_instance(args_positionals_requireds);
    if (co_args_idx == -1) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    if (!emit(PUSH, co_args_idx, Xen_AST_Node_STA(node))) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    Xen_Instance* body = Xen_AST_Node_Get_Child(node, 1);
    if (!body) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    CALLABLE_ptr code = compiler_ast(body, Xen_COMPILE_FUNCTION);
    if (!code) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    Xen_ssize_t co_code_idx = co_push_callable(code);
    if (co_code_idx == -1) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
    if (!emit(MAKE_FUNCTION, co_code_idx, Xen_AST_Node_STA(node))) {
      Xen_IGC_XPOP(roots);
      return 0;
    }
  }
  Xen_IGC_XPOP(roots);
  return 1;
}

Xen_Instance* compile_expr_function_arg_assigment(Compiler* c,
                                                  Xen_Instance* node) {
  if (Xen_AST_Node_Name_Cmp(node, "Assignment") != 0 ||
      Xen_AST_Node_Children_Size(node) != 2) {
    return NULL;
  }
  Xen_Instance* rhs = Xen_AST_Node_Get_Child(node, 1);
  if (Xen_AST_Node_Name_Cmp(rhs, "Expr") == 0) {
    if (!compile_expr(c, rhs)) {
      return NULL;
    }
  } else if (Xen_AST_Node_Name_Cmp(rhs, "FunctionExpr") == 0) {
    if (!compile_expr_function(c, rhs)) {
      return 0;
    }
  } else {
    return NULL;
  }
  Xen_Instance* lhs_expr = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_expr, "Expr") != 0 ||
      Xen_AST_Node_Children_Size(lhs_expr) != 1) {
    return NULL;
  }
  Xen_Instance* lhs_primary = Xen_AST_Node_Get_Child(lhs_expr, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_primary, "Primary") != 0 ||
      Xen_AST_Node_Children_Size(lhs_primary) != 1) {
    return NULL;
  }
  Xen_Instance* lhs_literal = Xen_AST_Node_Get_Child(lhs_primary, 0);
  if (Xen_AST_Node_Name_Cmp(lhs_literal, "Literal") != 0) {
    return NULL;
  }
  Xen_Instance* name = Xen_String_From_CString(Xen_AST_Node_Value(lhs_literal));
  if (!name) {
    return NULL;
  }
  return name;
}

int compile_expr_list(Compiler* c, Xen_Instance* node) {
  int error;
  Xen_Instance* constant_value = compile_expr_constant(&error, node);
  if (constant_value) {
    Xen_ssize_t co_idx = co_push_instance(constant_value);
    if (co_idx == -1) {
      return 0;
    }
    if (Xen_AST_Node_Value_Cmp(node, "tuple") == 0) {
      if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
        return 0;
      }
      return 1;
    } else if (Xen_AST_Node_Value_Cmp(node, "vector") == 0) {
      if (!emit(PUSH, co_idx, Xen_AST_Node_STA(node))) {
        return 0;
      }
      if (!emit(MAKE_VECTOR_FROM_ITERABLE, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
      return 1;
    } else {
      return 0;
    }
  } else if (error == 0) {
    for (Xen_size_t idx = 0; idx < Xen_AST_Node_Children_Size(node); idx++) {
      Xen_Instance* expr = Xen_AST_Node_Get_Child(node, idx);
      if (Xen_AST_Node_Name_Cmp(expr, "Binary") == 0) {
        if (!compile_expr_binary(c, expr)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr, "Unary") == 0) {
        if (!compile_expr_unary(c, expr)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
        if (!compile_expr_primary(c, expr)) {
          return 0;
        }
      } else {
        return 0;
      }
    }
    if (Xen_AST_Node_Value_Cmp(node, "tuple") == 0) {
      if (!emit(MAKE_TUPLE, Xen_AST_Node_Children_Size(node),
                Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Value_Cmp(node, "vector") == 0) {
      if (!emit(MAKE_VECTOR, Xen_AST_Node_Children_Size(node),
                Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else {
      return 0;
    }
    return 1;
  }
  return 0;
}

int compile_assignment(Compiler* c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) != 2) {
    return 0;
  }
  Xen_Instance* rhs = Xen_AST_Node_Get_Child(node, 1);
  if (Xen_AST_Node_Name_Cmp(rhs, "Expr") == 0) {
    if (!compile_expr(c, rhs)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(rhs, "FunctionExpr") == 0) {
    if (!compile_expr_function(c, rhs)) {
      return 0;
    }
  } else {
    return 0;
  }
  Xen_Instance* lhs = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(lhs, "Expr") != 0) {
    return 0;
  }
  if (!compile_assignment_expr(c, lhs)) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr(Compiler* c, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
    if (!compile_assignment_expr_primary(c, expr)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(expr, "List") == 0) {
    if (!compile_assignment_expr_list(c, expr)) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary(Compiler* c, Xen_Instance* node) {
  if (COMPILE_MODE == Xen_COMPILE_IMPLEMENT) {
    if (Xen_AST_Node_Children_Size(node) == 1) {
      Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
      if (!primary) {
        return 0;
      }
      if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
        if (!compile_assignment_expr_primary_literal(c, primary)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
        if (!compile_assignment_expr_primary_property(c, primary)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
        if (!compile_assignment_expr_primary_parent(c, primary)) {
          return 0;
        }
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  } else {
    if (Xen_AST_Node_Children_Size(node) == 2) {
      Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
      if (Xen_AST_Node_Name_Cmp(primary, "String") == 0) {
        if (!compile_expr_primary_string(c, primary)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(primary, "Number") == 0) {
        if (!compile_expr_primary_number(c, primary)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
        if (!compile_expr_primary_literal(c, primary)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
        if (!compile_expr_primary_property(c, primary)) {
          return 0;
        }
      } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
        if (!compile_expr_primary_parent(c, primary)) {
          return 0;
        }
      } else {
        return 0;
      }
      Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 1);
      if (Xen_AST_Node_Name_Cmp(suffix, "Suffix") == 0) {
        if (!compile_assignment_expr_primary_suffix(c, suffix)) {
          return 0;
        }
      } else {
        return 0;
      }
      return 1;
    }
    Xen_Instance* primary = Xen_AST_Node_Get_Child(node, 0);
    if (!primary) {
      return 0;
    }
    if (Xen_AST_Node_Name_Cmp(primary, "Literal") == 0) {
      if (!compile_assignment_expr_primary_literal(c, primary)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Property") == 0) {
      if (!compile_assignment_expr_primary_property(c, primary)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(primary, "Parent") == 0) {
      if (!compile_assignment_expr_primary_parent(c, primary)) {
        return 0;
      }
    } else {
      return 0;
    }
  }
  return 1;
}

int compile_assignment_expr_primary_literal(Compiler* c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(STORE, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_property(Compiler* c, Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(STORE_PROP, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_parent(Compiler* c, Xen_Instance* node) {
  Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
  if (!expr) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(expr, "Expr") == 0) {
    if (!compile_assignment_expr(c, expr)) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
  return 0;
}

int compile_assignment_expr_primary_suffix(Compiler* c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) == 2) {
    Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 0);
    if (Xen_AST_Node_Name_Cmp(suffix, "Call") == 0) {
      if (!compile_expr_primary_suffix_call(c, suffix)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(suffix, "Index") == 0) {
      if (!compile_expr_primary_suffix_index(c, suffix)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
      if (!compile_expr_primary_suffix_attr(c, suffix)) {
        return 0;
      }
    } else {
      return 0;
    }
    Xen_Instance* suffix2 = Xen_AST_Node_Get_Child(node, 1);
    if (Xen_AST_Node_Name_Cmp(suffix2, "Suffix") == 0) {
      if (!compile_assignment_expr_primary_suffix(c, suffix2)) {
        return 0;
      }
    } else {
      return 0;
    }
    return 1;
  }
  Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 0);
  if (!suffix) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(suffix, "Index") == 0) {
    if (!compile_assignment_expr_primary_suffix_index(c, suffix)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(suffix, "Attr") == 0) {
    if (!compile_assignment_expr_primary_suffix_attr(c, suffix)) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_suffix_index(Compiler* c,
                                                 Xen_Instance* node) {
  Xen_Instance* index = Xen_AST_Node_Get_Child(node, 0);
  if (!index) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(index, "Expr") == 0) {
    if (!compile_expr(c, index)) {
      return 0;
    }
  } else {
    return 0;
  }
  if (!emit(STORE_INDEX, 0, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_primary_suffix_attr(Compiler* c,
                                                Xen_Instance* node) {
  Xen_ssize_t co_idx = co_push_name(Xen_AST_Node_Value(node));
  if (co_idx == -1) {
    return 0;
  }
  if (!emit(STORE_ATTR, co_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_assignment_expr_list(Compiler* c, Xen_Instance* node) {
  int starred = 0;
  Xen_ssize_t starred_index = -1;
  Xen_size_t count = Xen_AST_Node_Children_Size(node);
  for (Xen_size_t i = 0; i < count; i++) {
    Xen_Instance* expr = Xen_AST_Node_Get_Child(node, i);
    if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
      continue;
    } else if (Xen_AST_Node_Name_Cmp(expr, "List") == 0) {
      continue;
    } else if (Xen_AST_Node_Name_Cmp(expr, "Unary") == 0) {
      if (starred) {
        return 0;
      }
      if (Xen_AST_Node_Value_Cmp(expr, "*") == 0) {
        starred = 1;
        starred_index = i;
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }
  if (starred) {
    if (starred_index == 0) {
      if (!emit(LIST_UNPACK_END, count, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (starred_index == (Xen_ssize_t)count - 1) {
      if (!emit(LIST_UNPACK_START, count, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else {
      if (!emit(LIST_UNPACK_END, (count - starred_index),
                Xen_AST_Node_STA(node))) {
        return 0;
      }
      if (!emit(LIST_UNPACK_START, (starred_index + 1),
                Xen_AST_Node_STA(node))) {
        return 0;
      }
    }
  } else {
    if (!emit(LIST_UNPACK, count, Xen_AST_Node_STA(node))) {
      return 0;
    }
  }
  for (Xen_size_t i = 0; i < count; i++) {
    Xen_Instance* expr = Xen_AST_Node_Get_Child(node, i);
    if (Xen_AST_Node_Name_Cmp(expr, "Primary") == 0) {
      if (!compile_assignment_expr_primary(c, expr)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr, "List") == 0) {
      if (!compile_assignment_expr_list(c, expr)) {
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(expr, "Unary") == 0) {
      Xen_Instance* primary = Xen_AST_Node_Get_Child(expr, 0);
      if (!primary) {
        return 0;
      }
      if (Xen_AST_Node_Name_Cmp(primary, "Primary") != 0) {
        return 0;
      }
      if (!compile_assignment_expr_primary(c, primary)) {
        return 0;
      }
    } else {
      return 0;
    }
  }
  return 1;
}

int compile_block(Compiler* c, Xen_Instance* node) {
  Xen_Instance* block = Xen_AST_Node_Get_Child(node, 0);
  if (!block) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(block, "Statement") == 0) {
    if (!compile_statement(c, block)) {
      return 0;
    }
  } else if (Xen_AST_Node_Name_Cmp(block, "StatementList") == 0) {
    if (!compile_statement_list(c, block)) {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_if_statement(Compiler* c, Xen_Instance* node) {
  Xen_Instance* condition = Xen_AST_Node_Get_Child(node, 0);
  if (!condition) {
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(condition, "Expr") != 0) {
    return 0;
  }
  if (!compile_expr(c, condition)) {
    return 0;
  }
  B_PTR if_false_block = B_NEW();
  if (!if_false_block) {
    return 0;
  }
  if (!emit_jump(JUMP_IF_FALSE, if_false_block, Xen_AST_Node_STA(node))) {
    B_FREE(if_false_block);
    return 0;
  }
  Xen_Instance* then = Xen_AST_Node_Get_Child(node, 1);
  if (!then) {
    B_FREE(if_false_block);
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(then, "Block") != 0) {
    B_FREE(if_false_block);
    return 0;
  }
  if (!compile_block(c, then)) {
    B_FREE(if_false_block);
    return 0;
  }
  if (Xen_AST_Node_Children_Size(node) == 3) {
    B_PTR end_block = B_NEW();
    if (!end_block) {
      B_FREE(if_false_block);
      return 0;
    }
    if (!emit_jump(JUMP, end_block, Xen_AST_Node_STA(node))) {
      B_FREE(end_block);
      B_FREE(if_false_block);
      return 0;
    }
    if (!B_LIST_PUSH(if_false_block)) {
      B_FREE(end_block);
      B_FREE(if_false_block);
      return 0;
    }
    B_SET_CURRENT(if_false_block);
    Xen_Instance* els = Xen_AST_Node_Get_Child(node, 2);
    if (Xen_AST_Node_Name_Cmp(els, "IfStatement") == 0) {
      if (!compile_if_statement(c, els)) {
        B_FREE(end_block);
        return 0;
      }
    } else if (Xen_AST_Node_Name_Cmp(els, "Block") == 0) {
      if (!compile_block(c, els)) {
        B_FREE(end_block);
        return 0;
      }
    } else {
      B_FREE(end_block);
      return 0;
    }
    if (!B_LIST_PUSH(end_block)) {
      B_FREE(end_block);
      return 0;
    }
    B_SET_CURRENT(end_block);
  } else {
    if (!B_LIST_PUSH(if_false_block)) {
      B_FREE(if_false_block);
      return 0;
    }
    B_SET_CURRENT(if_false_block);
  }
  return 1;
}

int compile_while_statement(Compiler* c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) != 2) {
    return 0;
  }
  B_PTR init_block = B_MAKE_CURRENT();
  if (!init_block) {
    return 0;
  }
  Xen_Instance* condition = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(condition, "Expr") != 0) {
    return 0;
  }
  if (!compile_expr(c, condition)) {
    return 0;
  }
  B_PTR if_false_block = B_NEW();
  if (!if_false_block) {
    return 0;
  }
  if (!emit_jump(JUMP_IF_FALSE, if_false_block, Xen_AST_Node_STA(node))) {
    B_FREE(if_false_block);
    return 0;
  }
  Xen_Instance* wdo = Xen_AST_Node_Get_Child(node, 1);
  if (!wdo) {
    B_FREE(if_false_block);
    return 0;
  }
  if (Xen_AST_Node_Name_Cmp(wdo, "Block") != 0) {
    B_FREE(if_false_block);
    return 0;
  }
  if (!CCS_PUSH(((CC){if_false_block, init_block}))) {
    B_FREE(if_false_block);
    return 0;
  }
  if (!compile_block(c, wdo)) {
    B_FREE(if_false_block);
    CCS_POP;
    return 0;
  }
  CCS_POP;
  if (!emit_jump(JUMP, init_block, Xen_AST_Node_STA(node))) {
    B_FREE(if_false_block);
    return 0;
  }
  if (!B_LIST_PUSH(if_false_block)) {
    B_FREE(if_false_block);
    return 0;
  }
  B_SET_CURRENT(if_false_block);
  return 1;
}

int compile_for_statement(Compiler* c, Xen_Instance* node) {
  if (Xen_AST_Node_Children_Size(node) != 3) {
    return 0;
  }
  Xen_Instance* iterable = Xen_AST_Node_Get_Child(node, 1);
  if (Xen_AST_Node_Name_Cmp(iterable, "Expr") != 0) {
    return 0;
  }
  if (!compile_expr(c, iterable)) {
    return 0;
  }
  if (!emit(ITER_GET, 0, Xen_AST_Node_STA(node))) {
    return 0;
  }
  B_PTR for_block = B_MAKE_CURRENT();
  if (!for_block) {
    return 0;
  }
  if (!emit(COPY, 0, Xen_AST_Node_STA(node))) {
    return 0;
  }
  B_PTR end_block = B_NEW();
  if (!end_block) {
    return 0;
  }
  if (!emit_jump(ITER_FOR, end_block, Xen_AST_Node_STA(node))) {
    B_FREE(end_block);
    return 0;
  }
  Xen_Instance* target = Xen_AST_Node_Get_Child(node, 0);
  if (Xen_AST_Node_Name_Cmp(target, "Expr") != 0) {
    B_FREE(end_block);
    return 0;
  }
  if (!compile_assignment_expr(c, target)) {
    B_FREE(end_block);
    return 0;
  }
  Xen_Instance* block = Xen_AST_Node_Get_Child(node, 2);
  if (Xen_AST_Node_Name_Cmp(block, "Block") != 0) {
    B_FREE(end_block);
    return 0;
  }
  if (!CCS_PUSH(((CC){end_block, for_block}))) {
    B_FREE(end_block);
    return 0;
  }
  if (!compile_block(c, block)) {
    B_FREE(end_block);
    CCS_POP;
    return 0;
  }
  CCS_POP;
  if (!emit_jump(JUMP, for_block, Xen_AST_Node_STA(node))) {
    B_FREE(end_block);
    return 0;
  }
  if (!B_LIST_PUSH(end_block)) {
    B_FREE(end_block);
    return 0;
  }
  B_SET_CURRENT(end_block);
  if (!emit(POP, 2, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int compile_flow_statement(Compiler* c, Xen_Instance* node) {
  if (Xen_AST_Node_Value_Cmp(node, "break") == 0) {
    CCS_ptr current = CCS_TOP;
    while (current && !current->context.b_break) {
      current = current->next;
    }
    if (current) {
      if (!emit_jump(JUMP, current->context.b_break, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else {
      return 0;
    }
  } else if (Xen_AST_Node_Value_Cmp(node, "continue") == 0) {
    CCS_ptr current = CCS_TOP;
    while (current && !current->context.b_continue) {
      current = current->next;
    }
    if (current) {
      if (!emit_jump(JUMP, CC_TOP.b_continue, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else {
      return 0;
    }
  } else {
    return 0;
  }
  return 1;
}

int compile_return_statement(Compiler* c, Xen_Instance* node) {
  if (COMPILE_MODE == Xen_COMPILE_FUNCTION) {
    if (Xen_AST_Node_Children_Size(node) == 0) {
      if (!emit(RETURN, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else if (Xen_AST_Node_Children_Size(node) == 1) {
      Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
      if (Xen_AST_Node_Name_Cmp(expr, "Expr") != 0) {
        return 0;
      }
      if (!compile_expr(c, expr)) {
        return 0;
      }
      if (!emit(RETURN_TOP, 0, Xen_AST_Node_STA(node))) {
        return 0;
      }
    } else {
      return 0;
    }
    return 1;
  }
  return 0;
}

int compile_implement_statement(Compiler* c, Xen_Instance* node) {
  Xen_Instance* name = Xen_String_From_CString(Xen_AST_Node_Value(node));
  if (!name) {
    return 0;
  }
  Xen_Instance* base = Xen_AST_Node_Get_Child(node, 0);
  if (!base) {
    return 0;
  }
  Xen_bool_t has_base = 0;
  if (Xen_AST_Node_Name_Cmp(base, "Base") != 0) {
    return 0;
  }
  if (Xen_AST_Node_Children_Size(base) == 1) {
    Xen_Instance* base_expr = Xen_AST_Node_Get_Child(base, 0);
    if (Xen_AST_Node_Name_Cmp(base_expr, "Expr") != 0) {
      return 0;
    }
    if (!compile_expr(c, base_expr)) {
      return 0;
    }
    has_base = 1;
  }
  Xen_ssize_t co_name_idx = co_push_instance(name);
  if (co_name_idx == -1) {
    return 0;
  }
  if (!emit(PUSH, co_name_idx, Xen_AST_Node_STA(node))) {
    return 0;
  }
  Xen_Instance* body = Xen_AST_Node_Get_Child(node, 1);
  if (!body) {
    return 0;
  }
  CALLABLE_ptr code = compiler_ast(body, Xen_COMPILE_IMPLEMENT);
  if (!code) {
    return 0;
  }
  Xen_ssize_t co_code_idx = co_push_callable(code);
  if (co_code_idx == -1) {
    return 0;
  }
  if (has_base) {
    if (!emit(BUILD_IMPLEMENT, co_code_idx, Xen_AST_Node_STA(node))) {
      return 0;
    }
  } else {
    if (!emit(BUILD_IMPLEMENT_NBASE, co_code_idx, Xen_AST_Node_STA(node))) {
      return 0;
    }
  }
  Xen_ssize_t local_name = co_push_name(Xen_AST_Node_Value(node));
  if (local_name == -1) {
    return 0;
  }
  if (!emit(STORE, local_name, Xen_AST_Node_STA(node))) {
    return 0;
  }
  return 1;
}

int ast_compile(block_list_ptr b_list, block_node_ptr* b_current, uint8_t mode,
                Xen_Instance* ast) {
  struct CompileContext_Stack* loop_stack = NULL;
  Compiler c = {b_list, b_current, &loop_stack, 0, mode};
  if (!compile_program(&c, ast)) {
    assert(loop_stack == NULL);
    return 0;
  }
  assert(loop_stack == NULL);
  return 1;
}
