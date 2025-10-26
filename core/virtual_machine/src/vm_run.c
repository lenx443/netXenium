#include "vm_run.h"
#include "attrs.h"
#include "bytecode.h"
#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "program.h"
#include "run_ctx_instance.h"
#include "vm.h"
#include "vm_instructs.h"
#include "vm_stack.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_vector.h"

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

static void op_load_prop(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_name = Xen_Operator_Eval_Pair_Steal2(
      ctx->ctx_code->code.consts->c_names, Xen_Number_From_UInt(oparg),
      Xen_OPR_GET_INDEX);
  if_nil_eval(c_name) {
    ctx->ctx_running = 0;
    return;
  }
  Xen_Instance* inst =
      xen_register_prop_get(Xen_String_As_CString(c_name), ctx->ctx_id);
  if_nil_eval(inst) {
    Xen_DEL_REF(c_name);
    ctx->ctx_running = 0;
    return;
  }
  Xen_DEL_REF(c_name);
  vm_stack_push(&ctx->ctx_stack, inst);
  Xen_DEL_REF(inst);
}

static void op_call(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* args = Xen_Vector_New();
  if_nil_eval(args) {
    ctx->ctx_running = 0;
    return;
  }
  for (uint8_t idx = 0; idx < oparg; idx++) {
    Xen_Instance* arg = vm_stack_pop(&ctx->ctx_stack);
    if (!Xen_Vector_Push(args, arg)) {
      Xen_DEL_REF(args);
      ctx->ctx_running = 0;
      return;
    }
    Xen_DEL_REF(arg);
  }
  Xen_Instance* callable = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* ret =
      vm_call_native_function(Xen_TYPE(callable)->__callable, callable, args);
  if (!ret) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    ctx->ctx_running = 0;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, ret);
  Xen_DEL_REF(ret);
  Xen_DEL_REF(callable);
  Xen_DEL_REF(args);
}

static void op_binaryop(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* second = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* first = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* rsult = Xen_Operator_Eval_Pair(first, second, (Xen_Opr)oparg);
  if (!rsult) {
    Xen_DEL_REF(second);
    Xen_DEL_REF(first);
    ctx->ctx_running = 0;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, rsult);
  Xen_DEL_REF(rsult);
  Xen_DEL_REF(second);
  Xen_DEL_REF(first);
}

static void op_attr_get(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* attr = Xen_Operator_Eval_Pair_Steal2(
      ctx->ctx_code->code.consts->c_names, Xen_Number_From_UInt(oparg),
      Xen_OPR_GET_INDEX);
  Xen_Instance* result = Xen_Attr_Get(inst, attr);
  if (!result) {
    Xen_DEL_REF(inst);
    Xen_DEL_REF(attr);
    ctx->ctx_running = 0;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(inst);
  Xen_DEL_REF(attr);
}

static void (*Dispatcher[HALT])(RunContext_ptr, uint8_t) = {
    [PUSH] = op_push,           [POP] = op_pop,   [LOAD] = op_load,
    [LOAD_PROP] = op_load_prop, [CALL] = op_call, [BINARYOP] = op_binaryop,
    [ATTR_GET] = op_attr_get};

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
