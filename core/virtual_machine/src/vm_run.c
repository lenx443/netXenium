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
#include "xen_map.h"
#include "xen_method.h"
#include "xen_method_implement.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_typedefs.h"
#include "xen_vector.h"
#include <stdint.h>

static void op_push(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_inst =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_instances, oparg);
  if (!c_inst) {
    ctx->ctx_error = 1;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, c_inst);
  Xen_DEL_REF(c_inst);
}

static void op_pop(RunContext_ptr ctx, uint8_t _) {
  Xen_DEL_REF(vm_stack_pop(&ctx->ctx_stack));
}

static void op_load(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* inst =
      vm_get_instance(Xen_String_As_CString(c_name), ctx->ctx_id);
  if (!inst) {
    Xen_DEL_REF(c_name);
    ctx->ctx_error = 1;
    return;
  }
  Xen_DEL_REF(c_name);
  vm_stack_push(&ctx->ctx_stack, inst);
  Xen_DEL_REF(inst);
}

static void op_load_prop(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* inst =
      xen_register_prop_get(Xen_String_As_CString(c_name), ctx->ctx_id);
  if (!inst) {
    Xen_DEL_REF(c_name);
    ctx->ctx_error = 1;
    return;
  }
  Xen_DEL_REF(c_name);
  vm_stack_push(&ctx->ctx_stack, inst);
  Xen_DEL_REF(inst);
}

static void op_load_index(RunContext_ptr ctx, uint8_t _) {
  Xen_Instance* index = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* rsult = Xen_Attr_Index_Get(inst, index);
  if (!rsult) {
    Xen_DEL_REF(index);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, rsult);
  Xen_DEL_REF(rsult);
  Xen_DEL_REF(index);
  Xen_DEL_REF(inst);
}

static void op_load_attr(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* attr =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  Xen_Instance* result = Xen_Attr_Get(inst, attr);
  if (!result) {
    Xen_DEL_REF(inst);
    Xen_DEL_REF(attr);
    ctx->ctx_error = 1;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(inst);
  Xen_DEL_REF(attr);
}

static void op_store(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  if (!inst) {
    Xen_DEL_REF(c_name);
    ctx->ctx_error = 1;
    return;
  }
  if (!Xen_Map_Push_Pair(ctx->ctx_instances, (Xen_Map_Pair){c_name, inst})) {
    Xen_DEL_REF(c_name);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  Xen_DEL_REF(c_name);
  Xen_DEL_REF(inst);
}

static void op_store_prop(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  if (!xen_register_prop_set(Xen_String_As_CString(c_name), inst,
                             ctx->ctx_id)) {
    Xen_DEL_REF(c_name);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  Xen_DEL_REF(c_name);
  Xen_DEL_REF(inst);
}

static void op_store_index(RunContext_ptr ctx, uint8_t _) {
  Xen_Instance* index = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* value = vm_stack_pop(&ctx->ctx_stack);
  if (!Xen_Attr_Index_Set(inst, index, value)) {
    Xen_DEL_REF(value);
    Xen_DEL_REF(inst);
    Xen_DEL_REF(index);
    ctx->ctx_error = 1;
    return;
  }
  Xen_DEL_REF(value);
  Xen_DEL_REF(inst);
  Xen_DEL_REF(index);
}

static void op_store_attr(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* value = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* attr =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!Xen_Attr_Set(inst, attr, value)) {
    Xen_DEL_REF(inst);
    Xen_DEL_REF(attr);
    ctx->ctx_error = 1;
    return;
  }
  Xen_DEL_REF(inst);
  Xen_DEL_REF(attr);
}

static void op_call(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* args = Xen_Vector_New();
  if (!args) {
    ctx->ctx_error = 1;
    return;
  }
  for (uint8_t idx = 0; idx < oparg; idx++) {
    Xen_Instance* arg = vm_stack_pop(&ctx->ctx_stack);
    if (!Xen_Vector_Push(args, arg)) {
      Xen_DEL_REF(args);
      ctx->ctx_error = 1;
      return;
    }
    Xen_DEL_REF(arg);
  }
  Xen_Instance* callable = vm_stack_pop(&ctx->ctx_stack);
  if (Xen_IMPL(callable)->__callable == NULL) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* ret =
      vm_call_native_function(Xen_IMPL(callable)->__callable, callable, args);
  if (!ret) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    ctx->ctx_error = 1;
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
    ctx->ctx_error = 1;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, rsult);
  Xen_DEL_REF(rsult);
  Xen_DEL_REF(second);
  Xen_DEL_REF(first);
}

static void op_unary_positive(RunContext_ptr ctx, uint8_t _) {
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__positive");
  if (!method) {
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil);
  if (!result) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(method);
  Xen_DEL_REF(inst);
}

static void op_unary_negative(RunContext_ptr ctx, uint8_t _) {
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__negative");
  if (!method) {
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil);
  if (!result) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(method);
  Xen_DEL_REF(inst);
}

static void op_unary_not(RunContext_ptr ctx, uint8_t _) {
  Xen_Instance* inst = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__not");
  if (!method) {
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil);
  if (!result) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ctx->ctx_error = 1;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(method);
  Xen_DEL_REF(inst);
}

static void (*Dispatcher[HALT])(RunContext_ptr, uint8_t) = {
    [PUSH] = op_push,
    [POP] = op_pop,
    [LOAD] = op_load,
    [LOAD_PROP] = op_load_prop,
    [LOAD_INDEX] = op_load_index,
    [LOAD_ATTR] = op_load_attr,
    [STORE] = op_store,
    [STORE_PROP] = op_store_prop,
    [STORE_INDEX] = op_store_index,
    [STORE_ATTR] = op_store_attr,
    [CALL] = op_call,
    [BINARYOP] = op_binaryop,
    [UNARY_POSITIVE] = op_unary_positive,
    [UNARY_NEGATIVE] = op_unary_negative,
    [UNARY_NOT] = op_unary_not,
};

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
#ifndef NDEBUG
    const char* previous_op = "No-OP";
    Xen_ssize_t previous_offset = -1;
#endif
    while (ctx->ctx_running && !ctx->ctx_error &&
           ctx->ctx_ip < pc.code->bc_size && !program.closed) {
      bc_Instruct_t instr = pc.code->bc_array[ctx->ctx_ip++];
      if (instr.bci_opcode >= HALT) {
        ctx->ctx_error = 1;
        break;
      }
#ifndef NDEBUG
      previous_op = Instruct_Info_Table[instr.bci_opcode].name;
      previous_offset = ctx->ctx_ip - 1;
#endif
      Dispatcher[instr.bci_opcode](ctx, instr.bci_oparg);
    }
    if (ctx->ctx_error) {
#ifndef NDEBUG
      printf("VM Error: opcode '%s'; offset %ld;\n", previous_op,
             previous_offset);
#endif
      ctx->ctx_running = 0;
      ctx->ctx_error = 0;
      vm_stack_free(&ctx->ctx_stack);
      return NULL;
    }
    ctx->ctx_running = 0;
    ctx->ctx_error = 0;
    vm_stack_free(&ctx->ctx_stack);
    return nil;
  }
  return NULL;
}
