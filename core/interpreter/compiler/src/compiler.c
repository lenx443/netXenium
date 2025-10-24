#include "compiler.h"
#include "bc_instruct.h"
#include "block_list.h"
#include "instance.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_instructs.h"
#include "xen_ast.h"
#include "xen_nil.h"

CALLABLE_ptr compiler(const char* text_code) {
  if (!text_code) {
    return NULL;
  }
  Lexer lexer = {text_code, 0};
  Parser parser = {&lexer, {0, "\0"}};
  parser_next(&parser);
  Xen_Instance* ast_program = parser_program(&parser);
  if_nil_eval(ast_program) return 0;
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
  struct Frame {
    Xen_Instance* node;
    size_t idx;
  } stack[1024];
  typedef struct Frame Frame;
  size_t sp = 0;
  stack[sp++] = (Frame){ast, 0};
  Frame Error = (Frame){Xen_AST_Node_New("CompilerError", NULL), 0};
  if_nil_eval(Error.node) {
    return 0;
  }
  while (sp > 0) {
    Frame frame = stack[--sp];
    Xen_Instance* node = frame.node;
    if (Xen_AST_Node_Name_Cmp(node, "CompilerError") == 0) {
      Xen_DEL_REF(Error.node);
      return 0;
    } else if (Xen_AST_Node_Name_Cmp(node, "Program") == 0) {
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
    } else if (Xen_AST_Node_Name_Cmp(node, "StatementList") == 0) {
      if (frame.idx < Xen_AST_Node_Children_Size(node)) {
        Xen_Instance* stmt = Xen_AST_Node_Get_Child(node, frame.idx++);
        if (Xen_AST_Node_Name_Cmp(stmt, "Statement") != 0) {
          Xen_DEL_REF(stmt);
          stack[sp++] = Error;
        }
        stack[sp++] = (Frame){stmt, 0};
        Xen_DEL_REF(stmt);
        continue;
      }
      stack[sp++] = Error;
    } else if (Xen_AST_Node_Name_Cmp(node, "Expr") == 0) {
      Xen_Instance* value = Xen_AST_Node_Get_Child(node, 0);
      if_nil_eval(value) {
        stack[sp++] = Error;
        continue;
      }
      if (Xen_AST_Node_Name_Cmp(value, "Primary") == 0 ||
          Xen_AST_Node_Name_Cmp(value, "Unary") == 0 ||
          Xen_AST_Node_Name_Cmp(value, "Binary") == 0 ||
          Xen_AST_Node_Name_Cmp(value, "List") == 0) {
        stack[sp++] = (Frame){value, 0};
        Xen_DEL_REF(value);
        continue;
      }
      stack[sp++] = Error;
    } else {
      stack[sp++] = Error;
    }
  }
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
                  current_block->instr_array->ir_array[instr_iterator].opcode,
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
    effect += Instruct_Stack_Effect_Table[inst.bci_opcode](inst.bci_oparg);
    if (effect > depth)
      depth = effect;
  }
  pc->stack_depth = depth;
}
