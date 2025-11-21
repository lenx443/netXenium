#include <stdint.h>
#include <stdio.h>

#include "attrs.h"
#include "bc_instruct.h"
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
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_method.h"
#include "xen_method_implement.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_tuple.h"
#include "xen_tuple_implement.h"
#include "xen_typedefs.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"

#define ERROR                                                                  \
  ctx->ctx_error = 1;                                                          \
  return;

#define JUMP(ip) ctx->ctx_ip = ip

#define STACK_PUSH(inst) vm_stack_push(ctx->ctx_stack, inst);
#define STACK_POP vm_stack_pop(ctx->ctx_stack)

static void op_nop([[maybe_unused]] RunContext_ptr ctx,
                   [[maybe_unused]] Xen_ulong_t oparg) {}

static void op_push(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* c_inst =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_instances, oparg);
  if (!c_inst) {
    ERROR;
  }
  STACK_PUSH(c_inst);
  Xen_DEL_REF(c_inst);
}

static void op_pop(RunContext_ptr ctx, Xen_ulong_t oparg) {
  for (uint8_t i = 0; i < oparg; i++) {
    Xen_DEL_REF(STACK_POP);
  }
}

static void op_load(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ERROR;
  }
  Xen_Instance* inst =
      Xen_VM_Load_Instance(Xen_String_As_CString(c_name), ctx->ctx_id);
  if (!inst) {
    Xen_DEL_REF(c_name);
    ERROR;
  }
  Xen_DEL_REF(c_name);
  STACK_PUSH(inst);
  Xen_DEL_REF(inst);
}

static void op_load_prop(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ERROR;
  }
  Xen_Instance* inst =
      xen_register_prop_get(Xen_String_As_CString(c_name), ctx->ctx_id);
  if (!inst) {
    Xen_DEL_REF(c_name);
    ERROR;
  }
  Xen_DEL_REF(c_name);
  STACK_PUSH(inst);
  Xen_DEL_REF(inst);
}

static void op_load_index(RunContext_ptr ctx,
                          [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* index = STACK_POP;
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* rsult = Xen_Attr_Index_Get(inst, index);
  if (!rsult) {
    Xen_DEL_REF(index);
    Xen_DEL_REF(inst);
    ERROR;
  }
  STACK_PUSH(rsult);
  Xen_DEL_REF(rsult);
  Xen_DEL_REF(index);
  Xen_DEL_REF(inst);
}

static void op_load_attr(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* attr =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  Xen_Instance* result = Xen_Attr_Get(inst, attr);
  if (!result) {
    Xen_DEL_REF(inst);
    Xen_DEL_REF(attr);
    ERROR;
  }
  STACK_PUSH(result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(inst);
  Xen_DEL_REF(attr);
}

static void op_store(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ERROR;
  }
  Xen_Instance* inst = STACK_POP;
  if (!inst) {
    Xen_DEL_REF(c_name);
    ERROR;
  }
  if (!Xen_Map_Push_Pair(ctx->ctx_instances, (Xen_Map_Pair){c_name, inst})) {
    Xen_DEL_REF(c_name);
    Xen_DEL_REF(inst);
    ERROR;
  }
  Xen_DEL_REF(c_name);
  Xen_DEL_REF(inst);
}

static void op_store_prop(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* c_name =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ERROR;
  }
  Xen_Instance* inst = STACK_POP;
  if (!xen_register_prop_set(Xen_String_As_CString(c_name), inst,
                             ctx->ctx_id)) {
    Xen_DEL_REF(c_name);
    Xen_DEL_REF(inst);
    ERROR;
  }
  Xen_DEL_REF(c_name);
  Xen_DEL_REF(inst);
}

static void op_store_index(RunContext_ptr ctx,
                           [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* index = STACK_POP;
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* value = STACK_POP;
  if (!Xen_Attr_Index_Set(inst, index, value)) {
    Xen_DEL_REF(value);
    Xen_DEL_REF(inst);
    Xen_DEL_REF(index);
    ERROR;
  }
  Xen_DEL_REF(value);
  Xen_DEL_REF(inst);
  Xen_DEL_REF(index);
}

static void op_store_attr(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* value = STACK_POP;
  Xen_Instance* attr =
      Xen_Attr_Index_Size_Get(ctx->ctx_code->code.consts->c_names, oparg);
  if (!Xen_Attr_Set(inst, attr, value)) {
    Xen_DEL_REF(value);
    Xen_DEL_REF(inst);
    Xen_DEL_REF(attr);
    ERROR;
  }
  Xen_DEL_REF(value);
  Xen_DEL_REF(inst);
  Xen_DEL_REF(attr);
}

static void op_make_tuple(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance** vals_array = Xen_Alloc(oparg * sizeof(Xen_Instance*));
  if (!vals_array) {
    ERROR;
  }
  for (uint8_t idx = oparg; idx > 0; --idx) {
    Xen_Instance* val = STACK_POP;
    vals_array[idx - 1] = val;
  }
  Xen_Instance* tuple = Xen_Tuple_From_Array(oparg, vals_array);
  if (!tuple) {
    for (uint8_t idx = 0; idx < oparg; idx++) {
      Xen_DEL_REF(vals_array[idx]);
    }
    Xen_Dealloc(vals_array);
    ERROR;
  }
  for (uint8_t idx = 0; idx < oparg; idx++) {
    Xen_DEL_REF(vals_array[idx]);
  }
  Xen_Dealloc(vals_array);
  STACK_PUSH(tuple);
  Xen_DEL_REF(tuple);
}

static void op_make_vector(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance** vals_array = Xen_Alloc(oparg * sizeof(Xen_Instance*));
  if (!vals_array) {
    ERROR;
  }
  for (uint8_t idx = oparg; idx > 0; --idx) {
    Xen_Instance* val = STACK_POP;
    vals_array[idx - 1] = val;
  }
  Xen_Instance* vector = Xen_Vector_From_Array(oparg, vals_array);
  if (!vector) {
    for (uint8_t idx = 0; idx < oparg; idx++) {
      Xen_DEL_REF(vals_array[idx]);
    }
    Xen_Dealloc(vals_array);
    ERROR;
  }
  for (uint8_t idx = 0; idx < oparg; idx++) {
    Xen_DEL_REF(vals_array[idx]);
  }
  Xen_Dealloc(vals_array);
  STACK_PUSH(vector);
  Xen_DEL_REF(vector);
}

static void op_make_vector_from_iterable(RunContext_ptr ctx,
                                         [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* iterable = STACK_POP;
  Xen_Instance* iter = Xen_Attr_Iter(iterable);
  if (!iter) {
    Xen_DEL_REF(iterable);
    ERROR;
  }
  Xen_DEL_REF(iterable);
  Xen_Instance* vector = Xen_Vector_New();
  if (!vector) {
    Xen_DEL_REF(iter);
    ERROR;
  }
  Xen_Instance* value = NULL;
  while ((value = Xen_Attr_Next(iter)) != NULL) {
    if (!Xen_Vector_Push(vector, value)) {
      Xen_DEL_REF(vector);
      Xen_DEL_REF(value);
      Xen_DEL_REF(iter);
      ERROR;
    }
    Xen_DEL_REF(value);
  }
  Xen_DEL_REF(iter);
  STACK_PUSH(vector);
  Xen_DEL_REF(vector);
}

static void op_make_map(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* map = Xen_Map_New();
  if (!map) {
    ERROR;
  }
  for (Xen_uint8_t i = 0; i < oparg; i++) {
    Xen_Instance* value = STACK_POP;
    Xen_Instance* key = STACK_POP;
    if (!Xen_Map_Push_Pair(map, (Xen_Map_Pair){key, value})) {
      Xen_DEL_REF(key);
      Xen_DEL_REF(value);
      Xen_DEL_REF(map);
      ERROR;
    }
    Xen_DEL_REF(key);
    Xen_DEL_REF(value);
  }
  STACK_PUSH(map);
  Xen_DEL_REF(map);
}

static void op_call(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance** args_array = Xen_Alloc(oparg * sizeof(Xen_Instance*));
  if (!args_array) {
    ERROR;
  }
  for (uint8_t idx = oparg; idx > 0; --idx) {
    Xen_Instance* arg = STACK_POP;
    args_array[idx - 1] = arg;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(oparg, args_array);
  if (!args) {
    for (uint8_t idx = 0; idx < oparg; idx++) {
      Xen_DEL_REF(args_array[idx]);
    }
    Xen_Dealloc(args_array);
    ERROR;
  }
  for (uint8_t idx = 0; idx < oparg; idx++) {
    Xen_DEL_REF(args_array[idx]);
  }
  Xen_Dealloc(args_array);
  Xen_Instance* callable = STACK_POP;
  if (Xen_IMPL(callable)->__callable == NULL) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    ERROR;
  }
  Xen_Instance* ret = Xen_VM_Call_Native_Function(
      Xen_IMPL(callable)->__callable, callable, args, nil);
  if (!ret) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    ERROR;
  }
  STACK_PUSH(ret);
  Xen_DEL_REF(ret);
  Xen_DEL_REF(callable);
  Xen_DEL_REF(args);
}

static void op_call_kw(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* kw_names = STACK_POP;
  Xen_Instance* kwargs = Xen_Map_New();
  if (!kwargs) {
    Xen_DEL_REF(kw_names);
    ERROR;
  }
  for (Xen_size_t idx = Xen_SIZE(kw_names); idx > 0; --idx) {
    Xen_Instance* arg = STACK_POP;
    Xen_Instance* kw_name = Xen_Attr_Index_Size_Get(kw_names, idx - 1);
    if (!Xen_Map_Push_Pair(kwargs, (Xen_Map_Pair){kw_name, arg})) {
      Xen_DEL_REF(kw_name);
      Xen_DEL_REF(arg);
      Xen_DEL_REF(kwargs);
      Xen_DEL_REF(kw_names);
      ERROR;
    }
    Xen_DEL_REF(kw_name);
    Xen_DEL_REF(arg);
  }
  Xen_size_t args_count = oparg - Xen_SIZE(kw_names);
  Xen_DEL_REF(kw_names);
  Xen_Instance** args_array = Xen_Alloc(args_count * sizeof(Xen_Instance*));
  if (!args_array) {
    Xen_DEL_REF(kwargs);
    ERROR;
  }
  for (uint8_t idx = args_count; idx > 0; --idx) {
    Xen_Instance* arg = STACK_POP;
    args_array[idx - 1] = arg;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(args_count, args_array);
  if (!args) {
    Xen_DEL_REF(kwargs);
    for (uint8_t idx = 0; idx < args_count; idx++) {
      Xen_DEL_REF(args_array[idx]);
    }
    Xen_Dealloc(args_array);
    ERROR;
  }
  for (uint8_t idx = 0; idx < args_count; idx++) {
    Xen_DEL_REF(args_array[idx]);
  }
  Xen_Dealloc(args_array);
  Xen_Instance* callable = STACK_POP;
  if (Xen_IMPL(callable)->__callable == NULL) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    Xen_DEL_REF(kwargs);
    ERROR;
  }
  Xen_Instance* ret = Xen_VM_Call_Native_Function(
      Xen_IMPL(callable)->__callable, callable, args, kwargs);
  if (!ret) {
    Xen_DEL_REF(callable);
    Xen_DEL_REF(args);
    Xen_DEL_REF(kwargs);
    ERROR;
  }
  STACK_PUSH(ret);
  Xen_DEL_REF(ret);
  Xen_DEL_REF(callable);
  Xen_DEL_REF(args);
  Xen_DEL_REF(kwargs);
}

static void op_binaryop(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* second = STACK_POP;
  Xen_Instance* first = STACK_POP;
  Xen_Instance* rsult = Xen_Operator_Eval_Pair(first, second, (Xen_Opr)oparg);
  if (!rsult) {
    Xen_DEL_REF(second);
    Xen_DEL_REF(first);
    ERROR;
  }
  STACK_PUSH(rsult);
  Xen_DEL_REF(rsult);
  Xen_DEL_REF(second);
  Xen_DEL_REF(first);
}

static void op_unary_positive(RunContext_ptr ctx,
                              [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__positive");
  if (!method) {
    Xen_DEL_REF(inst);
    ERROR;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ERROR;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
  if (!result) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ERROR;
  }
  STACK_PUSH(result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(method);
  Xen_DEL_REF(inst);
}

static void op_unary_negative(RunContext_ptr ctx,
                              [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__negative");
  if (!method) {
    Xen_DEL_REF(inst);
    ERROR;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ERROR;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
  if (!result) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ERROR;
  }
  STACK_PUSH(result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(method);
  Xen_DEL_REF(inst);
}

static void op_unary_not(RunContext_ptr ctx,
                         [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__not");
  if (!method) {
    Xen_DEL_REF(inst);
    ERROR;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ERROR;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
  if (!result) {
    Xen_DEL_REF(method);
    Xen_DEL_REF(inst);
    ERROR;
  }
  STACK_PUSH(result);
  Xen_DEL_REF(result);
  Xen_DEL_REF(method);
  Xen_DEL_REF(inst);
}

static void op_copy(RunContext_ptr ctx, [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* val = STACK_POP;
  STACK_PUSH(val);
  STACK_PUSH(val);
  Xen_DEL_REF(val);
}

static void op_print_top(RunContext_ptr ctx,
                         [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* val = STACK_POP;
  STACK_PUSH(val);
  if_nil_eval(val) {
    return;
  }
  const char* val_str = Xen_Attr_Raw_Str(val);
  if (val_str) {
    puts(val_str);
    Xen_Dealloc((void*)val_str);
  }
  Xen_DEL_REF(val);
}

static void op_jump(RunContext_ptr ctx, Xen_ulong_t oparg) {
  JUMP(oparg);
}

static void op_jump_if_true(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* cond = STACK_POP;
  Xen_Instance* evl = Xen_Attr_Boolean(cond);
  if (!evl) {
    ctx->ctx_error = 1;
  }
  if (evl == Xen_True) {
    JUMP(oparg);
  }
  Xen_DEL_REF(cond);
}

static void op_jump_if_false(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* cond = STACK_POP;
  Xen_Instance* evl = Xen_Attr_Boolean(cond);
  if (!evl) {
    ctx->ctx_error = 1;
  }
  if (evl == Xen_False) {
    JUMP(oparg);
  }
  Xen_DEL_REF(cond);
}

static void op_iter_get(RunContext_ptr ctx,
                        [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* iterable = STACK_POP;
  Xen_Instance* iter = Xen_Attr_Iter(iterable);
  if (!iter) {
    Xen_DEL_REF(iterable);
    ERROR;
  }
  Xen_DEL_REF(iterable);
  STACK_PUSH(iter);
  Xen_DEL_REF(iter);
}

static void op_iter_for(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* iter = STACK_POP;
  Xen_Instance* rsult = Xen_Attr_Next(iter);
  if (!rsult) {
    Xen_DEL_REF(iter);
    STACK_PUSH(nil);
    JUMP(oparg);
    return;
  }
  Xen_DEL_REF(iter);
  STACK_PUSH(rsult);
  Xen_DEL_REF(rsult);
}

static void op_seq_unpack(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* seq = STACK_POP;
  if (Xen_IMPL(seq) != &Xen_Tuple_Implement &&
      Xen_IMPL(seq) != &Xen_Vector_Implement) {
    Xen_DEL_REF(seq);
    ERROR;
  }
  if (oparg != Xen_SIZE(seq)) {
    Xen_DEL_REF(seq);
    ERROR;
  }
  for (uint8_t i = oparg; i > 0; --i) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i - 1);
    if (!val) {
      Xen_DEL_REF(seq);
      ERROR;
    }
    STACK_PUSH(val);
    Xen_DEL_REF(val);
  }
  Xen_DEL_REF(seq);
}

static void op_seq_unpack_start(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* seq = STACK_POP;
  if (Xen_IMPL(seq) != &Xen_Tuple_Implement &&
      Xen_IMPL(seq) != &Xen_Vector_Implement) {
    Xen_DEL_REF(seq);
    ERROR;
  }
  Xen_size_t seq_size = Xen_SIZE(seq);
  if (oparg > seq_size) {
    Xen_DEL_REF(seq);
    ERROR;
  }
  Xen_Instance* new_seq = Xen_Vector_New();
  if (!new_seq) {
    Xen_DEL_REF(seq);
    ERROR;
  }
  for (Xen_size_t i = oparg; i <= seq_size; i++) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i - 1);
    if (!val) {
      Xen_DEL_REF(new_seq);
      Xen_DEL_REF(seq);
      ERROR;
    }
    if (!Xen_Vector_Push(new_seq, val)) {
      Xen_DEL_REF(val);
      Xen_DEL_REF(new_seq);
      Xen_DEL_REF(seq);
      ERROR;
    }
    Xen_DEL_REF(val);
  }
  STACK_PUSH(new_seq);
  Xen_DEL_REF(new_seq);
  for (Xen_size_t i = oparg - 1; i > 0; --i) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i - 1);
    if (!val) {
      Xen_DEL_REF(seq);
      ERROR;
    }
    STACK_PUSH(val);
    Xen_DEL_REF(val);
  }
  Xen_DEL_REF(seq);
}

static void op_seq_unpack_end(RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* seq = STACK_POP;
  if (Xen_IMPL(seq) != &Xen_Tuple_Implement &&
      Xen_IMPL(seq) != &Xen_Vector_Implement) {
    Xen_DEL_REF(seq);
    ERROR;
  }
  Xen_size_t seq_size = Xen_SIZE(seq);
  if (oparg > seq_size) {
    Xen_DEL_REF(seq);
    ERROR;
  }
  for (Xen_size_t i = seq_size; i-- > (Xen_size_t)seq_size - (oparg - 1);) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i);
    if (!val) {
      Xen_DEL_REF(seq);
      ERROR;
    }
    STACK_PUSH(val);
    Xen_DEL_REF(val);
  }
  Xen_Instance* new_seq = Xen_Vector_New();
  if (!new_seq) {
    Xen_DEL_REF(seq);
    ERROR;
  }
  for (Xen_size_t i = 0; i <= seq_size - oparg; i++) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i);
    if (!val) {
      Xen_DEL_REF(new_seq);
      Xen_DEL_REF(seq);
      ERROR;
    }
    if (!Xen_Vector_Push(new_seq, val)) {
      Xen_DEL_REF(val);
      Xen_DEL_REF(new_seq);
      Xen_DEL_REF(seq);
      ERROR;
    }
    Xen_DEL_REF(val);
  }
  STACK_PUSH(new_seq);
  Xen_DEL_REF(new_seq);
  Xen_DEL_REF(seq);
}

static void (*Dispatcher[HALT])(RunContext_ptr, Xen_ulong_t) = {
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
    [MAKE_VECTOR] = op_make_vector,
    [MAKE_VECTOR_FROM_ITERABLE] = op_make_vector_from_iterable,
    [MAKE_MAP] = op_make_map,
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
    [SEQ_UNPACK] = op_seq_unpack,
    [SEQ_UNPACK_START] = op_seq_unpack_start,
    [SEQ_UNPACK_END] = op_seq_unpack_end,
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
    ctx->ctx_stack = vm_stack_new(pc.stack_depth + 1);
    Xen_GC_Write_Field(ctx, &ctx->ctx_stack, vm_stack_new(pc.stack_depth + 1));
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
      Xen_ulong_t oparg = instr.bci_oparg;
      if (instr.bci_oparg == 0xFF) {
        if (pc.code->bc_size - ctx->ctx_ip < XEN_ULONG_SIZE) {
          ctx->ctx_error = 1;
          break;
        }
        oparg = 0;
        for (Xen_size_t i = 0; i < XEN_ULONG_SIZE; i++) {
          bc_Instruct_t extend_arg_instr = pc.code->bc_array[ctx->ctx_ip++];
          oparg |= ((Xen_ulong_t)extend_arg_instr.bci_oparg) << (8 * i);
          ctx->ctx_jump_offset++;
        }
      }
      Dispatcher[instr.bci_opcode](ctx, oparg);
    }
    if (ctx->ctx_error) {
#ifndef NDEBUG
      printf("VM Error: opcode '%s'; offset %ld;\n", previous_op,
             previous_offset);
#endif
      ctx->ctx_running = 0;
      ctx->ctx_error = 0;
      ctx->ctx_stack = NULL;
      return NULL;
    }
    ctx->ctx_running = 0;
    ctx->ctx_error = 0;
    ctx->ctx_stack = NULL;
    return nil;
  }
  return NULL;
}
