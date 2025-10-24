#include "vm_run.h"
#include "bytecode.h"
#include "instance.h"
#include "operators.h"
#include "program.h"
#include "run_ctx_instance.h"
#include "vm.h"
#include "vm_instructs.h"
#include "vm_stack.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"

static void op_push(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_inst = Xen_Operator_Eval_Pair_Steal2(
      ctx->ctx_code->code.consts->c_instances, Xen_Number_From_UInt(oparg),
      Xen_OPR_GET_INDEX);
  if_nil_eval(c_inst) {
    ctx->ctx_running = 0;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, c_inst);
  Xen_DEL_REF(c_inst);
}

static void op_pop(RunContext_ptr ctx, uint8_t _) {
  Xen_DEL_REF(vm_stack_pop(&ctx->ctx_stack));
}

static void op_load(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_name = Xen_Operator_Eval_Pair_Steal2(
      ctx->ctx_code->code.consts->c_names, Xen_Number_From_UInt(oparg),
      Xen_OPR_GET_INDEX);
  if_nil_eval(c_name) {
    ctx->ctx_running = 0;
    return;
  }
  Xen_Instance* inst =
      vm_get_instance(Xen_String_As_CString(c_name), ctx->ctx_id);
  if_nil_eval(inst) {
    Xen_DEL_REF(c_name);
    ctx->ctx_running = 0;
    return;
  }
  Xen_DEL_REF(c_name);
  vm_stack_push(&ctx->ctx_stack, inst);
  Xen_DEL_REF(inst);
}

static void (*Dispatcher[HALT])(RunContext_ptr, uint8_t) = {
    [PUSH] = op_push, [POP] = op_pop, [LOAD] = op_load};

Xen_Instance* vm_run_ctx(RunContext_ptr ctx) {
  if (!ctx || !ctx->ctx_code)
    return NULL;
  if (ctx->ctx_code->callable_type == CALL_NATIVE_FUNCTIIN) {
    Xen_Instance* ret = ctx->ctx_code->native_callable(
        ctx->ctx_id, ctx->ctx_self, ctx->ctx_args);
    return ret;
  } else if (ctx->ctx_code->callable_type == CALL_BYTECODE_PROGRAM) {
    ctx->ctx_running = 1;
    ProgramCode_t pc = ctx->ctx_code->code;
    if (!vm_stack_init(&ctx->ctx_stack, pc.stack_depth + 1)) {
      return NULL;
    }
    while (ctx->ctx_running && ctx->ctx_ip < pc.code->bc_size &&
           !program.closed) {
      bc_Instruct_t instr = pc.code->bc_array[ctx->ctx_ip++];
      if (instr.bci_opcode >= HALT) {
        ctx->ctx_running = 0;
        break;
      }
      Dispatcher[instr.bci_opcode](ctx, instr.bci_oparg);
    }
    vm_stack_free(&ctx->ctx_stack);
    return nil;
  }
  return NULL;
}
