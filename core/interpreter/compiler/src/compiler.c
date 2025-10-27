#include <stdint.h>
#include <stdio.h>

#include "bc_instruct.h"
#include "block_list.h"
#include "compiler.h"
#include "instance.h"
#include "ir_bytecode.h"
#include "lexer.h"
#include "operators.h"
#include "parser.h"
#include "vm.h"
#include "vm_consts.h"
#include "vm_def.h"
#include "vm_instructs.h"
#include "xen_ast.h"
#include "xen_nil.h"
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
  if_nil_eval(ast_program) {
#ifndef NDEBUG
    printf("Parser Error\n");
#endif
    return 0;
  }
#ifndef NDEBUG
  Xen_AST_Node_Print(ast_program);
#endif
  block_list_ptr blocks = block_list_new();
  if (!blocks) {
    Xen_DEL_REF(ast_program);
    return NULL;
  }
  block_node_ptr main_block = block_new();
  if (!main_block) {
    Xen_DEL_REF(ast_program);
    block_list_free(blocks);
    return NULL;
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

int ast_compile(block_list_ptr block_result, block_node_ptr* block,
                Xen_Instance* ast) {
  (void)block_result;
  (void)block;
  struct Emit_Value {
    uint8_t opcode, oparg;
  } emit_value = {0, 0};
  struct Frame {
    Xen_Instance* node;
    size_t passes;
  } stack[1024];
  typedef struct Emit_Value Emit_Value;
  typedef struct Frame Frame;
  size_t sp = 0;
  stack[sp++] = (Frame){ast, 0};
  Frame Error = (Frame){Xen_AST_Node_New("CompilerError", NULL), 0};
  if_nil_eval(Error.node) {
    return 0;
  }
  Frame Emit = (Frame){Xen_AST_Node_New("CompilerEmit", NULL), 0};
  if_nil_eval(Emit.node) {
    Xen_DEL_REF(Error.node);
    return 0;
  }
  while (sp > 0) {
    Frame* frame = &stack[sp - 1];
    Xen_Instance* node = frame->node;
    if (Xen_AST_Node_Name_Cmp(node, "CompilerError") == 0) {
      Xen_DEL_REF(Emit.node);
      Xen_DEL_REF(Error.node);
      return 0;
    } else if (Xen_AST_Node_Name_Cmp(node, "CompilerEmit") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      ir_add_instr(
          (*block)->instr_array,
          (IR_Instruct_t){emit_value.opcode, emit_value.oparg, NULL, 0});
      frame->passes++;
    } else if (Xen_AST_Node_Name_Cmp(node, "Program") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      Xen_Instance* stmts = Xen_AST_Node_Get_Child(node, 0);
      if_nil_eval(stmts) {
        stack[sp++] = Error;
        continue;
      }
      if (Xen_AST_Node_Name_Cmp(stmts, "StatementList") != 0 &&
          Xen_AST_Node_Name_Cmp(stmts, "Statement") != 0) {
        Xen_DEL_REF(stmts);
        stack[sp++] = Error;
        continue;
      }
      stack[sp++] = (Frame){stmts, 0};
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
        stack[sp++] = (Frame){stmt, 0};
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
      if_nil_eval(stmt) {
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
      stack[sp++] = (Frame){stmt, 0};
      Xen_DEL_REF(stmt);
      frame->passes++;
    } else if (Xen_AST_Node_Name_Cmp(node, "Expr") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
      if_nil_eval(value) {
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
      stack[sp++] = (Frame){value, 0};
      Xen_DEL_REF(value);
      frame->passes++;
    } else if (Xen_AST_Node_Name_Cmp(node, "Primary") == 0) {
      switch (frame->passes) {
      case 0: {
        Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
        if_nil_eval(value) {
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
        stack[sp++] = (Frame){value, 0};
        Xen_DEL_REF(value);
        frame->passes++;
        break;
      }
      case 1: {
        if (Xen_AST_Node_Children_Size(node) == 2) {
          Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 1);
          if_nil_eval(suffix) {
            stack[sp++] = Error;
            break;
          }
          if (Xen_AST_Node_Name_Cmp(suffix, "Suffix") != 0) {
            Xen_DEL_REF(suffix);
            stack[sp++] = Error;
            break;
          }
          stack[sp++] = (Frame){suffix, 0};
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
    } else if (Xen_AST_Node_Name_Cmp(node, "String") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      Xen_Instance* value = Xen_String_From_CString(Xen_AST_Node_Value(node));
      if_nil_eval(value) {
        stack[sp++] = Error;
        continue;
      }
      Xen_ssize_t co_idx = vm_consts_push_instance(block_result->consts, value);
      if (co_idx < 0) {
        Xen_DEL_REF(value);
        stack[sp++] = Error;
        continue;
      }
      Xen_DEL_REF(value);
      emit_value = (Emit_Value){PUSH, (uint8_t)co_idx};
      stack[sp++] = Emit;
      frame->passes++;
    } else if (Xen_AST_Node_Name_Cmp(node, "Number") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      Xen_Instance* value =
          Xen_Number_From_CString(Xen_AST_Node_Value(node), 0);
      if_nil_eval(value) {
        stack[sp++] = Error;
        continue;
      }
      int co_idx = vm_consts_push_instance(block_result->consts, value);
      if (co_idx < 0) {
        Xen_DEL_REF(value);
        stack[sp++] = Error;
        continue;
      }
      Xen_DEL_REF(value);
      emit_value = (Emit_Value){PUSH, (uint8_t)co_idx};
      stack[sp++] = Emit;
      frame->passes++;
    } else if (Xen_AST_Node_Name_Cmp(node, "Literal") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      int co_idx =
          vm_consts_push_name(block_result->consts, Xen_AST_Node_Value(node));
      if (co_idx < 0) {
        stack[sp++] = Error;
        continue;
      }
      emit_value = (Emit_Value){LOAD, (uint8_t)co_idx};
      stack[sp++] = Emit;
      frame->passes++;
    } else if (Xen_AST_Node_Name_Cmp(node, "Property") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      int co_idx =
          vm_consts_push_name(block_result->consts, Xen_AST_Node_Value(node));
      if (co_idx < 0) {
        stack[sp++] = Error;
        continue;
      }
      emit_value = (Emit_Value){LOAD_PROP, (uint8_t)co_idx};
      stack[sp++] = Emit;
      frame->passes++;
    } else if (Xen_AST_Node_Name_Cmp(node, "Parent") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      Xen_Instance* expr = Xen_AST_Node_Get_Child(node, 0);
      if_nil_eval(expr) {
        stack[sp++] = Error;
        continue;
      }
      if (Xen_AST_Node_Name_Cmp(expr, "Expr") != 0) {
        Xen_DEL_REF(expr);
        stack[sp++] = Error;
        continue;
      }
      stack[sp++] = (Frame){expr, 0};
      Xen_DEL_REF(expr);
      frame->passes++;
    } else if (Xen_AST_Node_Name_Cmp(node, "Suffix") == 0) {
      switch (frame->passes) {
      case 0: {
        Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
        if_nil_eval(value) {
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
        stack[sp++] = (Frame){value, 0};
        Xen_DEL_REF(value);
        frame->passes++;
        break;
      }
      case 1: {
        if (Xen_AST_Node_Children_Size(node) == 2) {
          Xen_Instance* suffix = Xen_AST_Node_Get_Child(node, 1);
          if_nil_eval(suffix) {
            stack[sp++] = Error;
            break;
          }
          if (Xen_AST_Node_Name_Cmp(suffix, "Suffix") != 0) {
            Xen_DEL_REF(suffix);
            stack[sp++] = Error;
            break;
          }
          stack[sp++] = (Frame){suffix, 0};
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
          stack[sp++] = (Frame){arg, 0};
          Xen_DEL_REF(arg);
        }
        frame->passes++;
        break;
      case 1:
        emit_value =
            (Emit_Value){CALL, (uint8_t)Xen_AST_Node_Children_Size(node)};
        stack[sp++] = Emit;
        frame->passes++;
        break;
      default:
        --sp;
        break;
      }
    } else if (Xen_AST_Node_Name_Cmp(node, "Index") == 0) {
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
        stack[sp++] = (Frame){index, 0};
        Xen_DEL_REF(index);
        frame->passes++;
        break;
      case 1:
        emit_value = (Emit_Value){BINARYOP, (uint8_t)Xen_OPR_GET_INDEX};
        stack[sp++] = Emit;
        frame->passes++;
        break;
      default:
        --sp;
        break;
      }
    } else if (Xen_AST_Node_Name_Cmp(node, "Attr") == 0) {
      if (frame->passes > 0) {
        --sp;
        continue;
      }
      Xen_ssize_t co_idx =
          vm_consts_push_name(block_result->consts, Xen_AST_Node_Value(node));
      if (co_idx < 0) {
        stack[sp++] = Error;
        continue;
      }
      emit_value = (Emit_Value){ATTR_GET, co_idx};
      stack[sp++] = Emit;
      frame->passes++;
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
        stack[sp++] = (Frame){val, 0};
        Xen_DEL_REF(val);
        frame->passes++;
        break;
      case 1:
        if (Xen_AST_Node_Value_Cmp(node, "+") == 0) {
          emit_value = (Emit_Value){UNARY_POSITIVE, 0};
          stack[sp++] = Emit;
          frame->passes++;
        } else if (Xen_AST_Node_Value_Cmp(node, "-") == 0) {
          emit_value = (Emit_Value){UNARY_NEGATIVE, 0};
          stack[sp++] = Emit;
          frame->passes++;
        } else if (Xen_AST_Node_Value_Cmp(node, "not") == 0) {
          emit_value = (Emit_Value){UNARY_NOT, 0};
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
    } else {
#ifndef NDEBUG
      printf("Invalid Node '%s'\n", Xen_AST_Node_Name(node));
#endif
      stack[sp++] = Error;
    }
  }
  Xen_DEL_REF(Emit.node);
  Xen_DEL_REF(Error.node);
  return 1;
}

int blocks_compiler(ProgramCode_t* code, block_list_ptr blocks) {
  if (!blocks) {
    return 0;
  }
  code->consts = vm_consts_from_values(blocks->consts->c_names,
                                       blocks->consts->c_instances);
  if (!code->consts) {
    return 0;
  }
  code->code = bc_new();
  if (!code->code) {
    vm_consts_free(code->consts);
    return 0;
  }
  block_node_ptr current_block = blocks->head;
  while (current_block) {
    if (!current_block->ready) {
      current_block = current_block->next;
      continue;
    }
    for (size_t instr_iterator = 0;
         instr_iterator < current_block->instr_array->ir_size;
         instr_iterator++) {
      if (!bc_add_instr(
              code->code,
              (bc_Instruct_t){{
                  current_block->instr_array->ir_array[instr_iterator].opcode,
                  current_block->instr_array->ir_array[instr_iterator].oparg,
              }})) {
        vm_consts_free(code->consts);
        bc_free(code->code);
        return 0;
      }
    }
    current_block = current_block->next;
  }
  return 1;
}

void blocks_linealizer(block_list_ptr blocks) {
  if (!blocks)
    return;
  block_node_ptr current_block = blocks->head;
  int n = 0;
  while (current_block) {
    for (size_t i = 0; i < current_block->instr_array->ir_size; i++)
      current_block->instr_array->ir_array[i].instr_num = n++;
    current_block->ready = 1;
    current_block = current_block->next;
  }
}

void program_stack_depth(ProgramCode_t* pc) {
  size_t depth = 0;
  size_t effect = 0;
  for (size_t i = 0; i < pc->code->bc_size; i++) {
    bc_Instruct_t inst = pc->code->bc_array[i];
    effect += Instruct_Info_Table[inst.bci_opcode].stack_effect(inst.bci_oparg);
    if (effect > depth)
      depth = effect;
  }
  pc->stack_depth = depth;
}
