#include "bytecode.h"
#include "implement.h"
#include "instance.h"
#include "logs.h"
#include "program.h"
#include "vm.h"
#include "vm_register.h"
#include "vm_run.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM RUN", msg, ##__VA_ARGS__)

#define REG_SET(rn, expr)                                                                \
  if (ctx->ctx_reg.point_flag[rn]) {                                                     \
    Xen_DEL_REF(ctx->ctx_reg.reg[rn]);                                                   \
    ctx->ctx_reg.point_flag[rn] = 0;                                                     \
  }                                                                                      \
  ctx->ctx_reg.reg[rn] = (reg_t)(expr);

#define REG_SET_INST(rn, expr)                                                           \
  if (ctx->ctx_reg.point_flag[rn] && ctx->ctx_reg.reg[rn] != (reg_t)expr) {              \
    Xen_DEL_REF(ctx->ctx_reg.reg[rn]);                                                   \
    ctx->ctx_reg.point_flag[rn] = 0;                                                     \
  }                                                                                      \
  ctx->ctx_reg.reg[rn] = (reg_t)(Xen_ADD_REF(expr));                                     \
  ctx->ctx_reg.point_flag[rn] = 1;

void vm_run_ctx(RunContext_ptr ctx) {
  if (!ctx || !ctx->ctx_code) return;
  if (ctx->ctx_code->callable_type == CALL_NATIVE_FUNCTIIN) {
    ctx->ctx_reg.reg[1] =
        ctx->ctx_code->native_callable(ctx->ctx_id, ctx->ctx_self, ctx->ctx_args);
    if (ctx->ctx_caller) {
      ((RunContext_ptr)ctx->ctx_caller)->ctx_reg.reg[1] = ctx->ctx_reg.reg[1];
    }
  } else if (ctx->ctx_code->callable_type == CALL_BYTECODE_PROGRAM) {
    static void *dispatch_table[] = {
        &&NOP,       &&FUN_CALL,   &&JUMP,      &&LOAD_IMM,      &&LOAD_INSTANCE,
        &&LOAD_NAME, &&LOAD_CONST, &&LOAD_PROP, &&MAKE_INSTANCE, &&HALT};
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
        error("Instancia no callable");
        Xen_DEL_REF(args);
        Xen_DEL_REF(cmd);
        ctx->ctx_running = 0;
        continue;
      }
      REG_SET(1, vm_call_native_function(Xen_TYPE(cmd)->__callable, cmd, args));
      int ret = ctx->ctx_reg.reg[1];
      if (!ret) {
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
      REG_SET(instr.bci_dst, instr.bci_src2);
      continue;
    LOAD_INSTANCE:
      if (instr.bci_dst >= ctx->ctx_reg.capacity) {
        ctx->ctx_running = 0;
        continue;
      }
      if (instr.bci_src2 >= Xen_Vector_Size(pc.consts->c_names)) {
        ctx->ctx_running = 0;
        continue;
      }
      Xen_Instance *inst_name = Xen_Vector_Peek_Index(pc.consts->c_names, instr.bci_src2);
      if_nil_eval(inst_name) {
        ctx->ctx_running = 0;
        continue;
      }
      Xen_Instance *inst = vm_get_instance(Xen_String_As_CString(inst_name), ctx->ctx_id);
      if_nil_eval(inst) {
        error("Instancia no encontrada");
        ctx->ctx_running = 0;
        continue;
      }
      REG_SET_INST(instr.bci_dst, inst);
      Xen_DEL_REF(inst);
      continue;
    LOAD_NAME:
      if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
        ctx->ctx_running = 0;
        continue;
      }
      if (instr.bci_src2 >= Xen_Vector_Size(pc.consts->c_names)) {
        ctx->ctx_running = 0;
        continue;
      }
      Xen_Instance *c_name = Xen_Vector_Get_Index(pc.consts->c_names, instr.bci_src2);
      if_nil_eval(c_name) {
        ctx->ctx_running = 0;
        continue;
      }
      REG_SET_INST(instr.bci_dst, c_name);
      Xen_DEL_REF(c_name);
      continue;
    LOAD_CONST:
      if (BC_REG_GET_VALUE(instr.bci_dst) >= ctx->ctx_reg.capacity) {
        ctx->ctx_running = 0;
        continue;
      }
      if (instr.bci_src2 >= Xen_Vector_Size(pc.consts->c_instances)) {
        ctx->ctx_running = 0;
        continue;
      }
      Xen_Instance *c_inst = Xen_Vector_Get_Index(pc.consts->c_instances, instr.bci_src2);
      if_nil_eval(c_inst) {
        ctx->ctx_running = 0;
        continue;
      }
      REG_SET_INST(instr.bci_dst, c_inst);
      Xen_DEL_REF(c_inst);
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
      continue;
    MAKE_INSTANCE:
      if (instr.bci_src1 >= ctx->ctx_reg.capacity ||
          instr.bci_src2 >= ctx->ctx_reg.capacity) {
        ctx->ctx_running = 0;
        continue;
      }
      Xen_Instance *key = (Xen_Instance *)ctx->ctx_reg.reg[instr.bci_src1];
      Xen_Instance *value = (Xen_Instance *)ctx->ctx_reg.reg[instr.bci_src2];
      if (!Xen_Map_Push_Pair(ctx->ctx_instances, (Xen_Map_Pair){key, value})) {
        ctx->ctx_running = 0;
        continue;
      }
      continue;
    HALT:
      ctx->ctx_running = 0;
      continue;
    }
    if (ctx->ctx_caller) {
      ((RunContext_ptr)ctx->ctx_caller)->ctx_reg.reg[1] = ctx->ctx_reg.reg[1];
    }
    for (size_t i = 0; i < ctx->ctx_reg.capacity; i++) {
      if (ctx->ctx_reg.point_flag[i]) Xen_DEL_REF(ctx->ctx_reg.reg[i]);
    }
    log_show_and_clear(NULL);
  }
}
