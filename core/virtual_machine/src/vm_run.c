#include "bytecode.h"
#include "implement.h"
#include "logs.h"
#include "program.h"
#include "properties.h"
#include "vm.h"
#include "vm_register.h"
#include "vm_run.h"
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
  if (ctx->ctx_reg.point_flag[rn]) {                                                     \
    Xen_DEL_REF(ctx->ctx_reg.reg[rn]);                                                   \
    ctx->ctx_reg.point_flag[rn] = 0;                                                     \
  }                                                                                      \
  ctx->ctx_reg.reg[rn] = (reg_t)(expr);                                                  \
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
    LOAD_STRING:
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
      ctx->ctx_reg.reg[instr.bci_dst] = (reg_t)c_name;
      REG_SET_INST(BC_REG_GET_VALUE(instr.bci_dst), c_name);
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
      REG_SET_INST(instr.bci_dst, prop_name);
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
