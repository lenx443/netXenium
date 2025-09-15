#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bc_instruct.h"
#include "bytecode.h"
#include "callable.h"
#include "instance.h"
#include "logs.h"
#include "program.h"
#include "program_code.h"
#include "properties.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "run_frame.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_register.h"
#include "xen_command_implement.h"
#include "xen_command_instance.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

Xen_Instance *vm_current_ctx() { return run_context_stack_peek_top(&vm->vm_ctx_stack); }

Xen_Instance *vm_root_ctx() { return (Xen_Instance *)vm->root_context; }

bool vm_define_native_command(Xen_Instance *inst_map, const char *name,
                              Xen_Native_Func fun) {
  Xen_INSTANCE *cmd_inst = __instance_new(&Xen_Command_Implement, nil, 0);
  if_nil_eval(cmd_inst) { return false; }
  struct Xen_Command_Instance *command_inst = (struct Xen_Command_Instance *)cmd_inst;
  command_inst->cmd_callable = callable_new_native(fun);
  if (!command_inst->cmd_callable) {
    Xen_DEL_REF(cmd_inst);
    return false;
  }
  if (!Xen_Map_Push_Pair_Str(inst_map, (Xen_Map_Pair_Str){name, cmd_inst})) {
    Xen_DEL_REF(cmd_inst);
    return false;
  }
  return true;
}

bool vm_call_native_function(Xen_Native_Func func, Xen_INSTANCE *self,
                             Xen_Instance *args) {
  if (!run_context_stack_push(&vm->vm_ctx_stack, vm_current_ctx(),
                              (Xen_Instance *)vm->root_context, self, args)) {
    return false;
  }
  if (!func(run_ctx_id(vm_current_ctx()), self, args)) {
    puts("pase");
    run_context_stack_pop_top(&vm->vm_ctx_stack);
    return false;
  }
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  return true;
}

bool vm_call_basic_native_function(Xen_Native_Func func, struct __Instance *self) {
  reg_t regs[128];
  uint8_t reg_flag[128];
  memset(regs, 0, 128);
  memset(reg_flag, 0, 128);
  struct RunContext run_ctx = {
      Xen_INSTANCE_SET(0, &Xen_Run_Frame, XEN_INSTANCE_FLAG_STATIC),
      .ctx_flags = CTX_FLAG_STATIC,
      .ctx_closure = (Xen_Instance *)vm->root_context,
      .ctx_caller = vm_current_ctx(),
      .ctx_self = self,
      .ctx_code = NULL,
      .ctx_reg =
          {
              .reg = regs,
              .point_flag = reg_flag,
              .capacity = 128,
          },
      .ctx_args = NULL,
      .ctx_instances = NULL,
      .ctx_ip = 0,
      .ctx_running = false,
  };
  Xen_ADD_REF(self);
  if (!run_context_stack_push_refer(&vm->vm_ctx_stack, &run_ctx)) {
    Xen_DEL_REF(self);
    return false;
  }
  Xen_ADD_REF(self);
  func(run_ctx_id(vm_current_ctx()), self, NULL);
  Xen_DEL_REF(self);
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  Xen_DEL_REF(self);
  return true;
}

Xen_INSTANCE *vm_get_instance(const char *name, ctx_id_t id) {
  if (!name || !VM_CHECK_ID(id)) { return nil; }
  RunContext_ptr current = (RunContext_ptr)vm_current_ctx();
  while (current) {
    Xen_Instance *inst = Xen_Map_Get_Str(current->ctx_instances, name);
    if_nil_neval(inst) { return inst; }
    printf("pase");
    current = (RunContext_ptr)current->ctx_closure;
  }
  return nil;
}

void vm_ctx_clear(RunContext_ptr ctx) {
  ctx->ctx_code = NULL;
  vm_register_clear(&ctx->ctx_reg);
  ctx->ctx_ip = 0;
  ctx->ctx_running = 0;
}

int vm_new_ctx_callable(CALLABLE_ptr callable, Xen_Instance *closure,
                        struct __Instance *self, Xen_Instance *args) {
  if (!callable) { return 0; }
  if (!run_context_stack_push(&vm->vm_ctx_stack,
                              run_context_stack_peek_top(&vm->vm_ctx_stack), closure,
                              self, args)) {
    return 0;
  }
  RunContext_ptr ctx = (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  ctx->ctx_code = callable;
  return 1;
}

int vm_run_callable(CALLABLE_ptr callable, Xen_Instance *closure, struct __Instance *self,
                    Xen_Instance *args) {
  if (!callable) { return 0; }
  if (!vm_new_ctx_callable(callable, closure, self, args)) { return 0; }
  vm_run_ctx((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack));
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  return 1;
}

void vm_run_ctx(RunContext_ptr ctx) {
  if (!ctx || !ctx->ctx_code) return;
  if (ctx->ctx_code->callable_type == CALL_NATIVE_FUNCTIIN) {
    ctx->ctx_reg.reg[1] =
        ctx->ctx_code->native_callable(ctx->ctx_id, ctx->ctx_self, ctx->ctx_args);
    if (ctx->ctx_caller) {
      ((RunContext_ptr)ctx->ctx_caller)->ctx_reg.reg[1] = ctx->ctx_reg.reg[1];
    }
  } else if (ctx->ctx_code->callable_type == CALL_BYTECODE_PROGRAM) {
    static void *dispatch_table[] = {&&NOP,         &&FUN_CALL,  &&JUMP, &&LOAD_IMM,
                                     &&LOAD_STRING, &&LOAD_PROP, &&HALT};
    ctx->ctx_running = 1;
    ProgramCode_t pc = ctx->ctx_code->code;
    while (ctx->ctx_running && ctx->ctx_ip < pc.code->bc_size && !program.closed) {
      bc_Instruct_t instr = pc.code->bc_array[ctx->ctx_ip++];
      if (instr.bci_opcode > OP_HALT) {
        ctx->ctx_running = 0;
        break;
      }

      goto *dispatch_table[instr.bci_opcode];
    NOP:
      continue;
    FUN_CALL: {
      if (!ctx->ctx_reg.point_flag[0]) {
        ctx->ctx_running = 0;
        continue;
      }
      Xen_Instance *fun_name = (Xen_Instance *)ctx->ctx_reg.reg[0];
      Xen_Instance *cmd = vm_get_instance(Xen_String_As_CString(fun_name), ctx->ctx_id);
      if_nil_eval(cmd) {
        error("Instancia no encontrada");
        ctx->ctx_running = 0;
        continue;
      }
      int arg_num = instr.bci_src2;
      Xen_Instance *args = Xen_Vector_New();
      if_nil_eval(args) {
        Xen_DEL_REF(cmd);
        ctx->ctx_running = 0;
        continue;
      }
      bool success = true;
      for (int i = 1; i <= arg_num; i++) {
        if (!ctx->ctx_reg.point_flag[i]) {
          success = false;
          break;
        }
        Xen_Instance *arg_val = (Xen_Instance *)ctx->ctx_reg.reg[i];
        if (!Xen_Vector_Push(args, arg_val)) {
          success = false;
          break;
        }
      }
      if (!success) {
        Xen_DEL_REF(args);
        Xen_DEL_REF(cmd);
        ctx->ctx_running = 0;
        continue;
      }
      if (!Xen_TYPE(cmd)->__callable) {
        Xen_DEL_REF(args);
        Xen_DEL_REF(cmd);
        ctx->ctx_running = 0;
        continue;
      }
      int ret = ctx->ctx_reg.reg[1] =
          vm_call_native_function(Xen_TYPE(cmd)->__callable, cmd, args);
      if (!ret) {
        printf("ret = %d\n", ret);
        Xen_DEL_REF(args);
        Xen_DEL_REF(cmd);
        ctx->ctx_running = 0;
        continue;
      }
      Xen_DEL_REF(args);
      Xen_DEL_REF(cmd);
      continue;
    }
    JUMP: {
      ctx->ctx_ip = instr.bci_src2;
      continue;
    }
    LOAD_IMM:
      if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
        ctx->ctx_running = 0;
        continue;
      }
      ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = instr.bci_src2;
      continue;
    LOAD_STRING:
      if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
        ctx->ctx_running = 0;
        continue;
      }
      if (instr.bci_src2 >= Xen_Vector_Size(pc.consts->c_names)) {
        ctx->ctx_running = 0;
        continue;
      }
      if (ctx->ctx_reg.point_flag[BC_REG_GET_VALUE(instr.bci_src2)]) {
        Xen_DEL_REF(ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_src2)]);
      }
      Xen_Instance *c_name = Xen_Vector_Get_Index(pc.consts->c_names, instr.bci_src2);
      if_nil_eval(c_name) {
        ctx->ctx_running = 0;
        continue;
      }
      ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (reg_t)c_name;
      ctx->ctx_reg.point_flag[BC_REG_GET_VALUE(instr.bci_dst)] = 1;
      continue;
    LOAD_PROP:
      if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
        ctx->ctx_running = 0;
        continue;
      }
      if (instr.bci_src2 >= Xen_Vector_Size(pc.consts->c_names)) {
        ctx->ctx_running = 0;
        continue;
      }
      const char *prop_key = Xen_String_As_CString(
          Xen_Vector_Peek_Index(pc.consts->c_names, instr.bci_src2));
      prop_struct *prop = prop_reg_value(prop_key, *prop_register, 1);
      if (!prop) {
        error("No se encontro la propiedad '%s' no se encontro", prop_key);
        ctx->ctx_running = 0;
        continue;
      }
      Xen_Instance *prop_name = Xen_String_From_CString(prop->value);
      if_nil_eval(prop_name) {
        ctx->ctx_running = 0;
        continue;
      }
      ctx->ctx_reg.reg[BC_REG_GET_VALUE(instr.bci_dst)] = (reg_t)prop_name;
      ctx->ctx_reg.point_flag[BC_REG_GET_VALUE(instr.bci_dst)] = 1;
      continue;
    HALT:
      ctx->ctx_running = 0;
      continue;
    }
    if (ctx->ctx_caller) {
      ((RunContext_ptr)ctx->ctx_caller)->ctx_reg.reg[1] = ctx->ctx_reg.reg[1];
    }
    for (int i = 0; i < ctx->ctx_reg.capacity; i++) {
      if (ctx->ctx_reg.point_flag[i]) Xen_DEL_REF(ctx->ctx_reg.reg[i]);
    }
    log_show_and_clear(NULL);
  }
}
