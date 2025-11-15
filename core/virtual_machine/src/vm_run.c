#include <stdint.h>
#include <stdio.h>

#include "attrs.h"
#include "bytecode.h"
#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "program.h"
#include "run_ctx_instance.h"
#include "vm.h"
#include "vm_instructs.h"
#include "vm_run.h"
#include "vm_stack.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_map.h"
#include "xen_method.h"
#include "xen_method_implement.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_tuple_implement.h"
#include "xen_typedefs.h"

static void op_nop([[maybe_unused]] RunContext_ptr ctx,
                   [[maybe_unused]] uint8_t oparg) {}

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

static void op_pop(RunContext_ptr ctx, uint8_t oparg) {
  for (uint8_t i = 0; i < oparg; i++) {
    Xen_DEL_REF(vm_stack_pop(&ctx->ctx_stack));
  }
}

static void op_load(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* inst =
      Xen_VM_Load_Instance(Xen_String_As_CString(c_name), ctx->ctx_id);
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

static void op_load_index(RunContext_ptr ctx, [[maybe_unused]] uint8_t oparg) {
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

static void op_store_index(RunContext_ptr ctx, [[maybe_unused]] uint8_t oparg) {
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
    Xen_DEL_REF(value);
    Xen_DEL_REF(inst);
    Xen_DEL_REF(attr);
    ctx->ctx_error = 1;
    return;
  }
  Xen_DEL_REF(value);
  Xen_DEL_REF(inst);
  Xen_DEL_REF(attr);
}

static void op_make_tuple(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance** vals_array = Xen_Alloc(oparg * sizeof(Xen_Instance*));
  if (!vals_array) {
    ctx->ctx_error = 1;
    return;
  }
  for (uint8_t idx = oparg; idx > 0; --idx) {
    Xen_Instance* val = vm_stack_pop(&ctx->ctx_stack);
    vals_array[idx - 1] = val;
  }
  Xen_Instance* tuple = Xen_Tuple_From_Array(oparg, vals_array);
  if (!tuple) {
    for (uint8_t idx = 0; idx < oparg; idx++) {
      Xen_DEL_REF(vals_array[idx]);
    }
    Xen_Dealloc(vals_array);
    ctx->ctx_error = 1;
    return;
  }
  for (uint8_t idx = 0; idx < oparg; idx++) {
    Xen_DEL_REF(vals_array[idx]);
  }
  Xen_Dealloc(vals_array);
  vm_stack_push(&ctx->ctx_stack, tuple);
  Xen_DEL_REF(tuple);
}

static void op_unpack_tuple(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* tuple = vm_stack_pop(&ctx->ctx_stack);
  if (Xen_IMPL(tuple) != &Xen_Tuple_Implement) {
    Xen_DEL_REF(tuple);
    ctx->ctx_error = 1;
    return;
  }
  if (oparg != Xen_SIZE(tuple)) {
    Xen_DEL_REF(tuple);
    ctx->ctx_error = 1;
    return;
  }
  for (uint8_t i = oparg; i > 0; --i) {
    Xen_Instance* val = Xen_Tuple_Get_Index(tuple, i - 1);
    vm_stack_push(&ctx->ctx_stack, val);
    Xen_DEL_REF(val);
  }
  Xen_DEL_REF(tuple);
}

static void op_call(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance** args_array = Xen_Alloc(oparg * sizeof(Xen_Instance*));
  if (!args_array) {
    ctx->ctx_error = 1;
    return;
  }
  for (uint8_t idx = oparg; idx > 0; --idx) {
    Xen_Instance* arg = vm_stack_pop(&ctx->ctx_stack);
    args_array[idx - 1] = arg;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(oparg, args_array);
  if (!args) {
    for (uint8_t idx = 0; idx < oparg; idx++) {
      Xen_DEL_REF(args_array[idx]);
    }
    Xen_Dealloc(args_array);
    ctx->ctx_error = 1;
    return;
  }
  for (uint8_t idx = 0; idx < oparg; idx++) {
    Xen_DEL_REF(args_array[idx]);
  }
  Xen_Dealloc(args_array);
  Xen_Instance* callable = vm_stack_pop(&ctx->ctx_stack);
  if (Xen_IMPL(callable)->__callable == NULL) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* ret = Xen_VM_Call_Native_Function(
      Xen_IMPL(callable)->__callable, callable, args, nil);
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

static void op_call_kw(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* kw_names = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* kwargs = Xen_Map_New();
  if (!kwargs) {
    Xen_DEL_REF(kw_names);
    ctx->ctx_error = 1;
    return;
  }
  for (Xen_size_t idx = Xen_SIZE(kw_names); idx > 0; --idx) {
    Xen_Instance* arg = vm_stack_pop(&ctx->ctx_stack);
    Xen_Instance* kw_name = Xen_Attr_Index_Size_Get(kw_names, idx - 1);
    if (!Xen_Map_Push_Pair(kwargs, (Xen_Map_Pair){kw_name, arg})) {
      Xen_DEL_REF(kw_name);
      Xen_DEL_REF(arg);
      Xen_DEL_REF(kwargs);
      Xen_DEL_REF(kw_names);
      ctx->ctx_error = 1;
      return;
    }
    Xen_DEL_REF(kw_name);
    Xen_DEL_REF(arg);
  }
  Xen_size_t args_count = oparg - Xen_SIZE(kw_names);
  Xen_DEL_REF(kw_names);
  Xen_Instance** args_array = Xen_Alloc(args_count * sizeof(Xen_Instance*));
  if (!args_array) {
    Xen_DEL_REF(kwargs);
    ctx->ctx_error = 1;
    return;
  }
  for (uint8_t idx = args_count; idx > 0; --idx) {
    Xen_Instance* arg = vm_stack_pop(&ctx->ctx_stack);
    args_array[idx - 1] = arg;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(args_count, args_array);
  if (!args) {
    Xen_DEL_REF(kwargs);
    for (uint8_t idx = 0; idx < args_count; idx++) {
      Xen_DEL_REF(args_array[idx]);
    }
    Xen_Dealloc(args_array);
    ctx->ctx_error = 1;
    return;
  }
  for (uint8_t idx = 0; idx < args_count; idx++) {
    Xen_DEL_REF(args_array[idx]);
  }
  Xen_Dealloc(args_array);
  Xen_Instance* callable = vm_stack_pop(&ctx->ctx_stack);
  if (Xen_IMPL(callable)->__callable == NULL) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    Xen_DEL_REF(kwargs);
    ctx->ctx_error = 1;
    return;
  }
  Xen_Instance* ret = Xen_VM_Call_Native_Function(
      Xen_IMPL(callable)->__callable, callable, args, kwargs);
  if (!ret) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    Xen_DEL_REF(kwargs);
    ctx->ctx_error = 1;
    return;
  }
  vm_stack_push(&ctx->ctx_stack, ret);
  Xen_DEL_REF(ret);
  Xen_DEL_REF(callable);
  Xen_DEL_REF(args);
  Xen_DEL_REF(kwargs);
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

static void op_unary_positive(RunContext_ptr ctx,
                              [[maybe_unused]] uint8_t oparg) {
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
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
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

static void op_unary_negative(RunContext_ptr ctx,
                              [[maybe_unused]] uint8_t oparg) {
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
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
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

static void op_unary_not(RunContext_ptr ctx, [[maybe_unused]] uint8_t oparg) {
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
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
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

static void op_copy(RunContext_ptr ctx, [[maybe_unused]] uint8_t oparg) {
  Xen_Instance* val = vm_stack_pop(&ctx->ctx_stack);
  vm_stack_push(&ctx->ctx_stack, val);
  vm_stack_push(&ctx->ctx_stack, val);
  Xen_DEL_REF(val);
}

static void op_print_top(RunContext_ptr ctx, [[maybe_unused]] uint8_t oparg) {
  Xen_Instance* val = vm_stack_pop(&ctx->ctx_stack);
  vm_stack_push(&ctx->ctx_stack, val);
  const char* val_str = Xen_Attr_Raw_Str(val);
  if (val_str) {
    puts(val_str);
    Xen_Dealloc((void*)val_str);
  }
  Xen_DEL_REF(val);
}

static void op_jump(RunContext_ptr ctx, uint8_t oparg) {
  ctx->ctx_ip = oparg;
}

static void op_jump_if_true(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* cond = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* evl = Xen_Attr_Boolean(cond);
  if (!evl) {
    ctx->ctx_error = 1;
  }
  if (evl == Xen_True) {
    ctx->ctx_ip = oparg;
  }
  Xen_DEL_REF(cond);
}

static void op_jump_if_false(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* cond = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* evl = Xen_Attr_Boolean(cond);
  if (!evl) {
    ctx->ctx_error = 1;
  }
  if (evl == Xen_False) {
    ctx->ctx_ip = oparg;
  }
  Xen_DEL_REF(cond);
}

static void op_iter_get(RunContext_ptr ctx, [[maybe_unused]] uint8_t oparg) {
  Xen_Instance* iterable = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* iter = Xen_Attr_Iter(iterable);
  if (!iter) {
    Xen_DEL_REF(iterable);
    ctx->ctx_error = 1;
    return;
  }
  Xen_DEL_REF(iterable);
  vm_stack_push(&ctx->ctx_stack, iter);
  Xen_DEL_REF(iter);
}

static void op_iter_for(RunContext_ptr ctx, uint8_t oparg) {
  Xen_Instance* iter = vm_stack_pop(&ctx->ctx_stack);
  Xen_Instance* rsult = Xen_Attr_Next(iter);
  if (!rsult) {
    Xen_DEL_REF(iter);
    vm_stack_push(&ctx->ctx_stack, nil);
    ctx->ctx_ip = oparg;
    return;
  }
  Xen_DEL_REF(iter);
  vm_stack_push(&ctx->ctx_stack, rsult);
  Xen_DEL_REF(rsult);
}

static void (*Dispatcher[HALT])(RunContext_ptr, uint8_t) = {
    [NOP] = op_nop,
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
    [MAKE_TUPLE] = op_make_tuple,
    [UNPACK_TUPLE] = op_unpack_tuple,
    [CALL] = op_call,
    [CALL_KW] = op_call_kw,
    [BINARYOP] = op_binaryop,
    [UNARY_POSITIVE] = op_unary_positive,
    [UNARY_NEGATIVE] = op_unary_negative,
    [UNARY_NOT] = op_unary_not,
    [COPY] = op_copy,
    [PRINT_TOP] = op_print_top,
    [JUMP] = op_jump,
    [JUMP_IF_TRUE] = op_jump_if_true,
    [JUMP_IF_FALSE] = op_jump_if_false,
    [ITER_GET] = op_iter_get,
    [ITER_FOR] = op_iter_for,
};

Xen_Instance* vm_run_ctx(RunContext_ptr ctx) {
  if (!ctx || !ctx->ctx_code)
    return NULL;
  if (ctx->ctx_code->callable_type == CALL_NATIVE_FUNCTIIN) {
    Xen_Instance* ret = ctx->ctx_code->native_callable(
        ctx->ctx_id, ctx->ctx_self, ctx->ctx_args, ctx->ctx_kwargs);
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
