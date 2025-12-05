#include <stdint.h>
#include <stdio.h>

#include "attrs.h"
#include "basic.h"
#include "basic_builder_implement.h"
#include "basic_builder_instance.h"
#include "basic_templates.h"
#include "bc_instruct.h"
#include "bytecode.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "program.h"
#include "run_ctx.h"
#include "run_ctx_instance.h"
#include "run_ctx_stack.h"
#include "source_file.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_instructs.h"
#include "vm_run.h"
#include "vm_stack.h"
#include "xen_alloc.h"
#include "xen_boolean.h"
#include "xen_cstrings.h"
#include "xen_except.h"
#include "xen_function.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_method.h"
#include "xen_method_implement.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_string.h"
#include "xen_string_implement.h"
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

static void op_nop([[maybe_unused]] VM_Run* vmr,
                   [[maybe_unused]] RunContext_ptr ctx,
                   [[maybe_unused]] Xen_ulong_t oparg) {}

static void op_push([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                    Xen_ulong_t oparg) {
  Xen_Instance* c_inst =
      Xen_Vector_Get_Index(ctx->ctx_code->code.consts->c_instances, oparg);
  if (!c_inst) {
    ERROR;
  }
  STACK_PUSH(c_inst);
}

static void op_pop([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                   Xen_ulong_t oparg) {
  for (uint8_t i = 0; i < oparg; i++) {
    STACK_POP;
  }
}

static void op_load([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                    Xen_ulong_t oparg) {
  Xen_Instance* c_name =
      Xen_Vector_Get_Index(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ERROR;
  }
  Xen_c_string_t name = Xen_String_As_CString(c_name);
  Xen_Instance* inst = Xen_VM_Load_Instance(name, ctx->ctx_id);
  if (!inst) {
    Xen_UndefName(name);
    ERROR;
  }
  STACK_PUSH(inst);
}

static void op_load_prop([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                         Xen_ulong_t oparg) {
  Xen_Instance* c_name =
      Xen_Vector_Get_Index(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ERROR;
  }
  Xen_c_string_t reg = Xen_String_As_CString(c_name);
  Xen_Instance* inst = xen_register_prop_get(reg, ctx->ctx_id);
  if (!inst) {
    Xen_UndefReg(reg);
    ERROR;
  }
  STACK_PUSH(inst);
}

static void op_load_index([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                          [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* index = STACK_POP;
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* rsult = Xen_Attr_Index_Get(inst, index);
  if (!rsult) {
    Xen_IndexError(index);
    ERROR;
  }
  STACK_PUSH(rsult);
}

static void op_load_attr([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                         Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* attr =
      Xen_Vector_Get_Index(ctx->ctx_code->code.consts->c_names, oparg);
  Xen_Instance* result = Xen_Attr_Get(inst, attr);
  if (!result) {
    Xen_AttrError(Xen_String_As_CString(attr));
    ERROR;
  }
  STACK_PUSH(result);
}

static void op_store([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                     Xen_ulong_t oparg) {
  Xen_Instance* c_name =
      Xen_Vector_Get_Index(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ERROR;
  }
  Xen_Instance* inst = STACK_POP;
  if (!inst) {
    ERROR;
  }
  Xen_IGC_Push(inst);
  if (!Xen_Map_Push_Pair(ctx->ctx_instances, (Xen_Map_Pair){c_name, inst})) {
    Xen_IGC_Pop();
    ERROR;
  }
  Xen_IGC_Pop();
}

static void op_store_prop([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                          Xen_ulong_t oparg) {
  Xen_Instance* c_name =
      Xen_Vector_Get_Index(ctx->ctx_code->code.consts->c_names, oparg);
  if (!c_name) {
    ERROR;
  }
  Xen_Instance* inst = STACK_POP;
  if (!xen_register_prop_set(Xen_String_As_CString(c_name), inst,
                             ctx->ctx_id)) {
    ERROR;
  }
}

static void op_store_index([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                           [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* index = STACK_POP;
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* value = STACK_POP;
  if (!Xen_Attr_Index_Set(inst, index, value)) {
    Xen_IndexError_Store(index);
    ERROR;
  }
}

static void op_store_attr([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                          Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* value = STACK_POP;
  Xen_Instance* attr =
      Xen_Vector_Get_Index(ctx->ctx_code->code.consts->c_names, oparg);
  if (!Xen_Attr_Set(inst, attr, value)) {
    Xen_AttrError_Store(Xen_String_As_CString(attr));
    ERROR;
  }
}

static void op_make_tuple([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                          Xen_ulong_t oparg) {
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
    }
    Xen_Dealloc(vals_array);
    ERROR;
  }
  Xen_Dealloc(vals_array);
  STACK_PUSH(tuple);
}

static void op_make_vector([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                           Xen_ulong_t oparg) {
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
    Xen_Dealloc(vals_array);
    ERROR;
  }
  Xen_Dealloc(vals_array);
  STACK_PUSH(vector);
}

static void op_make_vector_from_iterable([[maybe_unused]] VM_Run* vmr,
                                         RunContext_ptr ctx,
                                         [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* iterable = STACK_POP;
  Xen_Instance* iter = Xen_Attr_Iter(iterable);
  if (!iter) {
    Xen_IterError(iterable);
    ERROR;
  }
  Xen_Instance* vector = Xen_Vector_New();
  if (!vector) {
    ERROR;
  }
  Xen_Instance* value = NULL;
  while ((value = Xen_Attr_Next(iter)) != NULL) {
    if (!Xen_Vector_Push(vector, value)) {
      ERROR;
    }
  }
  STACK_PUSH(vector);
}

static void op_make_map([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                        Xen_ulong_t oparg) {
  Xen_Instance* map = Xen_Map_New();
  if (!map) {
    ERROR;
  }
  Xen_IGC_Push(map);
  Xen_IGC_Fork* tmp = Xen_IGC_Fork_New();
  for (Xen_uint8_t i = 0; i < oparg; i++) {
    Xen_Instance* value = STACK_POP;
    Xen_Instance* key = STACK_POP;
    Xen_IGC_Fork_Push(tmp, key);
    Xen_IGC_Fork_Push(tmp, value);
    if (!Xen_Map_Push_Pair(map, (Xen_Map_Pair){key, value})) {
      ERROR;
    }
  }
  Xen_IGC_XPOP(2);
  STACK_PUSH(map);
}

static void op_make_function([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                             Xen_ulong_t oparg) {
  CALLABLE_ptr code =
      callable_vector_get(ctx->ctx_code->code.consts->c_callables, oparg);
  Xen_Instance* args_names = STACK_POP;
  Xen_Instance* args_deafult_values = STACK_POP;
  Xen_GC_Push_Root((Xen_GCHeader*)code);
  Xen_IGC_Push(args_names);
  Xen_IGC_Push(args_deafult_values);
  Xen_Instance* function = Xen_Function_From_Callable(
      code, (Xen_Instance*)ctx, args_names, args_deafult_values);
  if (!function) {
    Xen_IGC_XPOP(2);
    Xen_GC_Pop_Root();
    ERROR;
  }
  Xen_IGC_XPOP(2);
  STACK_PUSH(function);
  Xen_GC_Pop_Root();
}

static void op_make_function_nargs([[maybe_unused]] VM_Run* vmr,
                                   RunContext_ptr ctx, Xen_ulong_t oparg) {
  CALLABLE_ptr code =
      callable_vector_get(ctx->ctx_code->code.consts->c_callables, oparg);
  Xen_Instance* function =
      Xen_Function_From_Callable(code, (Xen_Instance*)ctx, nil, nil);
  if (!function) {
    ERROR;
  }
  STACK_PUSH(function);
}

static void op_call([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                    Xen_ulong_t oparg) {
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
    Xen_Dealloc(args_array);
    ERROR;
  }
  Xen_Dealloc(args_array);
  Xen_Instance* callable = STACK_POP;
  if (Xen_IMPL(callable)->__callable == NULL) {
    Xen_CallError_Impl(callable);
    ERROR;
  }
  Xen_IGC_Push(args);
  Xen_IGC_Push(callable);
  if (!Xen_IMPL(callable)->__callable(callable, args, nil)) {
    Xen_CallError(callable);
    Xen_IGC_XPOP(2);
    ERROR;
  }
  Xen_IGC_XPOP(2);
}

static void op_call_kw([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                       Xen_ulong_t oparg) {
  Xen_size_t roots = 0;
  Xen_Instance* kw_names = STACK_POP;
  Xen_Instance* kwargs = Xen_Map_New();
  if (!kwargs) {
    ERROR;
  }
  Xen_IGC_XPUSH(kwargs, roots);
  for (Xen_size_t idx = Xen_SIZE(kw_names); idx > 0; --idx) {
    Xen_Instance* arg = STACK_POP;
    Xen_Instance* kw_name = Xen_Attr_Index_Size_Get(kw_names, idx - 1);
    if (!Xen_Map_Push_Pair(kwargs, (Xen_Map_Pair){kw_name, arg})) {
      Xen_IGC_XPOP(roots);
      ERROR;
    }
  }
  Xen_size_t args_count = oparg - Xen_SIZE(kw_names);
  Xen_Instance** args_array = Xen_Alloc(args_count * sizeof(Xen_Instance*));
  if (!args_array) {
    Xen_IGC_XPOP(roots);
    ERROR;
  }
  for (uint8_t idx = args_count; idx > 0; --idx) {
    Xen_Instance* arg = STACK_POP;
    args_array[idx - 1] = arg;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(args_count, args_array);
  if (!args) {
    Xen_IGC_XPOP(roots);
    Xen_Dealloc(args_array);
    ERROR;
  }
  Xen_IGC_XPUSH(args, roots);
  Xen_Dealloc(args_array);
  Xen_Instance* callable = STACK_POP;
  if (Xen_IMPL(callable)->__callable == NULL) {
    Xen_CallError_Impl(callable);
    Xen_IGC_XPOP(roots);
    ERROR;
  }
  Xen_IGC_XPUSH(callable, roots);
  if (!Xen_IMPL(callable)->__callable(callable, args, kwargs)) {
    Xen_CallError(callable);
    Xen_IGC_XPOP(roots);
    ERROR;
  }
  Xen_IGC_XPOP(roots);
}

static void op_binaryop([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                        Xen_ulong_t oparg) {
  Xen_Instance* second = STACK_POP;
  Xen_Instance* first = STACK_POP;
  Xen_IGC_Push(first);
  Xen_IGC_Push(second);
  Xen_Instance* rsult = Xen_Operator_Eval_Pair(first, second, (Xen_Opr)oparg);
  if (!rsult) {
    Xen_OprError();
    Xen_IGC_XPOP(2);
    ERROR;
  }
  Xen_IGC_XPOP(2);
  STACK_PUSH(rsult);
}

static void op_unary_positive([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                              [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__positive");
  if (!method) {
    Xen_OprError();
    ERROR;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_OprError();
    ERROR;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
  if (!result) {
    Xen_OprError();
    ERROR;
  }
  STACK_PUSH(result);
}

static void op_unary_negative([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                              [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__negative");
  if (!method) {
    Xen_OprError();
    ERROR;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_OprError();
    ERROR;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
  if (!result) {
    Xen_OprError();
    ERROR;
  }
  STACK_PUSH(result);
}

static void op_unary_not([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                         [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* inst = STACK_POP;
  Xen_Instance* method = Xen_Attr_Get_Str(inst, "__not");
  if (!method) {
    Xen_OprError();
    ERROR;
  }
  if (Xen_IMPL(method) != &Xen_Method_Implement) {
    Xen_OprError();
    ERROR;
  }
  Xen_Instance* result = Xen_Method_Call(method, nil, nil);
  if (!result) {
    Xen_OprError();
    ERROR;
  }
  STACK_PUSH(result);
}

static void op_copy([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                    [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* val = STACK_POP;
  STACK_PUSH(val);
  STACK_PUSH(val);
}

static void op_print_top([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
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
}

static void op_jump([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                    Xen_ulong_t oparg) {
  JUMP(oparg);
}

static void op_throw([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                     [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* val = STACK_POP;
  if (!Xen_VM_Except_Throw(val)) {
    Xen_ThrowError();
    ERROR;
  }
}

static void op_jump_if_true([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                            Xen_ulong_t oparg) {
  Xen_Instance* cond = STACK_POP;
  Xen_Instance* evl = Xen_Attr_Boolean(cond);
  if (!evl) {
    ctx->ctx_error = 1;
  }
  if (evl == Xen_True) {
    JUMP(oparg);
  }
}

static void op_jump_if_false([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                             Xen_ulong_t oparg) {
  Xen_Instance* cond = STACK_POP;
  Xen_Instance* evl = Xen_Attr_Boolean(cond);
  if (!evl) {
    ctx->ctx_error = 1;
  }
  if (evl == Xen_False) {
    JUMP(oparg);
  }
}

static void op_iter_get([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                        [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* iterable = STACK_POP;
  Xen_Instance* iter = Xen_Attr_Iter(iterable);
  if (!iter) {
    Xen_IterError(iterable);
    ERROR;
  }
  STACK_PUSH(iter);
}

static void op_iter_for([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                        Xen_ulong_t oparg) {
  Xen_Instance* iter = STACK_POP;
  Xen_Instance* rsult = Xen_Attr_Next(iter);
  if (!rsult) {
    STACK_PUSH(nil);
    JUMP(oparg);
    return;
  }
  STACK_PUSH(rsult);
}

static void op_list_unpack([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                           Xen_ulong_t oparg) {
  Xen_Instance* seq = STACK_POP;
  if (Xen_IMPL(seq) != &Xen_Tuple_Implement &&
      Xen_IMPL(seq) != &Xen_Vector_Implement) {
    Xen_ListError(seq);
    ERROR;
  }
  if (oparg != Xen_SIZE(seq)) {
    ERROR;
  }
  for (uint8_t i = oparg; i > 0; --i) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i - 1);
    if (!val) {
      ERROR;
    }
    STACK_PUSH(val);
  }
}

static void op_list_unpack_start([[maybe_unused]] VM_Run* vmr,
                                 RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* seq = STACK_POP;
  if (Xen_IMPL(seq) != &Xen_Tuple_Implement &&
      Xen_IMPL(seq) != &Xen_Vector_Implement) {
    Xen_ListError(seq);
    ERROR;
  }
  Xen_size_t seq_size = Xen_SIZE(seq);
  if (oparg > seq_size) {
    ERROR;
  }
  Xen_Instance* new_seq = Xen_Vector_New();
  if (!new_seq) {
    ERROR;
  }
  for (Xen_size_t i = oparg; i <= seq_size; i++) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i - 1);
    if (!val) {
      ERROR;
    }
    if (!Xen_Vector_Push(new_seq, val)) {
      ERROR;
    }
  }
  STACK_PUSH(new_seq);
  for (Xen_size_t i = oparg - 1; i > 0; --i) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i - 1);
    if (!val) {
      ERROR;
    }
    STACK_PUSH(val);
  }
}

static void op_list_unpack_end([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                               Xen_ulong_t oparg) {
  Xen_Instance* seq = STACK_POP;
  if (Xen_IMPL(seq) != &Xen_Tuple_Implement &&
      Xen_IMPL(seq) != &Xen_Vector_Implement) {
    Xen_ListError(seq);
    ERROR;
  }
  Xen_size_t seq_size = Xen_SIZE(seq);
  if (oparg > seq_size) {
    ERROR;
  }
  for (Xen_size_t i = seq_size; i-- > (Xen_size_t)seq_size - (oparg - 1);) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i);
    if (!val) {
      ERROR;
    }
    STACK_PUSH(val);
  }
  Xen_Instance* new_seq = Xen_Vector_New();
  if (!new_seq) {
    ERROR;
  }
  for (Xen_size_t i = 0; i <= seq_size - oparg; i++) {
    Xen_Instance* val = Xen_Attr_Index_Size_Get(seq, i);
    if (!val) {
      ERROR;
    }
    if (!Xen_Vector_Push(new_seq, val)) {
      ERROR;
    }
  }
  STACK_PUSH(new_seq);
}

static void op_build_implement([[maybe_unused]] VM_Run* vmr, RunContext_ptr ctx,
                               Xen_ulong_t oparg) {
  CALLABLE_ptr code =
      callable_vector_get(ctx->ctx_code->code.consts->c_callables, oparg);
  Xen_Instance* name = STACK_POP;
  Xen_Instance* base = STACK_POP;
  Xen_Instance* builder =
      __instance_new(&Xen_Basic_Builder_Implement, nil, nil, 0);
  if (!builder) {
    ERROR;
  }
  if (Xen_IMPL(name) != &Xen_String_Implement || Xen_IMPL(base) != &Xen_Basic) {
    ERROR;
  }
  ((Xen_Basic_Builder*)builder)->name =
      Xen_CString_Dup(Xen_String_As_CString(name));
  ((Xen_Basic_Builder*)builder)->base = (Xen_Implement*)base;
  Xen_Instance* new_ctx = Xen_Ctx_New((Xen_Instance*)ctx, (Xen_Instance*)ctx,
                                      builder, nil, nil, NULL, code);
  if (!new_ctx) {
    ERROR;
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack, new_ctx)) {
    ERROR;
  }
}

static void op_build_implement_nbase([[maybe_unused]] VM_Run* vmr,
                                     RunContext_ptr ctx, Xen_ulong_t oparg) {
  CALLABLE_ptr code =
      callable_vector_get(ctx->ctx_code->code.consts->c_callables, oparg);
  Xen_Instance* name = STACK_POP;
  Xen_Instance* builder =
      __instance_new(&Xen_Basic_Builder_Implement, nil, nil, 0);
  if (!builder) {
    ERROR;
  }
  if (Xen_IMPL(name) != &Xen_String_Implement) {
    ERROR;
  }
  ((Xen_Basic_Builder*)builder)->name =
      Xen_CString_Dup(Xen_String_As_CString(name));
  Xen_Instance* new_ctx = Xen_Ctx_New((Xen_Instance*)ctx, (Xen_Instance*)ctx,
                                      builder, nil, nil, NULL, code);
  if (!new_ctx) {
    ERROR;
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack, new_ctx)) {
    ERROR;
  }
}

static void op_return(VM_Run* vmr, RunContext_ptr ctx, Xen_ulong_t oparg) {
  Xen_Instance* ret =
      Xen_Vector_Get_Index(ctx->ctx_code->code.consts->c_instances, oparg);
  Xen_size_t current_id = ctx->ctx_id;
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  RunContext_ptr ctx_top =
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  if (ctx_top && current_id > vmr->ctx_id) {
    vm_stack_push(ctx_top->ctx_stack, ret);
  } else {
    vmr->retval = ret;
  }
}

static void op_return_top(VM_Run* vmr, RunContext_ptr ctx,
                          [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Instance* ret = STACK_POP;
  Xen_size_t current_id = ctx->ctx_id;
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  RunContext_ptr ctx_top =
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  if (ctx_top && current_id > vmr->ctx_id) {
    vm_stack_push(ctx_top->ctx_stack, ret);
  } else {
    vmr->retval = ret;
  }
}

static void op_return_build_implement(VM_Run* vmr, RunContext_ptr ctx,
                                      [[maybe_unused]] Xen_ulong_t oparg) {
  Xen_Basic_Builder* builder = (Xen_Basic_Builder*)ctx->ctx_self;
  Xen_Instance* impl =
      Xen_Basic_New(builder->name, builder->__map, builder->base);
  if (!impl) {
    ERROR
  }
  Xen_size_t current_id = ctx->ctx_id;
  run_context_stack_pop_top(&vm->vm_ctx_stack);
  RunContext_ptr ctx_top =
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  if (ctx_top && current_id > vmr->ctx_id) {
    vm_stack_push(ctx_top->ctx_stack, impl);
  } else {
    vmr->retval = impl;
  }
}

static void (*Dispatcher[HALT])(VM_Run*, RunContext_ptr, Xen_ulong_t) = {
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
    [MAKE_FUNCTION] = op_make_function,
    [MAKE_FUNCTION_NARGS] = op_make_function_nargs,
    [CALL] = op_call,
    [CALL_KW] = op_call_kw,
    [BINARYOP] = op_binaryop,
    [UNARY_POSITIVE] = op_unary_positive,
    [UNARY_NEGATIVE] = op_unary_negative,
    [UNARY_NOT] = op_unary_not,
    [COPY] = op_copy,
    [PRINT_TOP] = op_print_top,
    [THROW] = op_throw,
    [JUMP] = op_jump,
    [JUMP_IF_TRUE] = op_jump_if_true,
    [JUMP_IF_FALSE] = op_jump_if_false,
    [ITER_GET] = op_iter_get,
    [ITER_FOR] = op_iter_for,
    [LIST_UNPACK] = op_list_unpack,
    [LIST_UNPACK_START] = op_list_unpack_start,
    [LIST_UNPACK_END] = op_list_unpack_end,
    [BUILD_IMPLEMENT] = op_build_implement,
    [BUILD_IMPLEMENT_NBASE] = op_build_implement_nbase,
    [RETURN] = op_return,
    [RETURN_TOP] = op_return_top,
    [RETURN_BUILD_IMPLEMENT] = op_return_build_implement,
};

static bc_Instruct_t vm_run_instruct(VM_Run* vmr, Xen_Instance* ctx_inst) {
  RunContext_ptr ctx = (RunContext_ptr)ctx_inst;
  bc_Instruct_t instr = ctx->ctx_code->code.code->bc_array[ctx->ctx_ip++];
  if (instr.hdr.bci_opcode >= HALT) {
    ctx->ctx_error = 1;
    return (bc_Instruct_t){{NOP, 0}, {0}};
  }
  Xen_ulong_t oparg = instr.hdr.bci_oparg;
  if (instr.hdr.bci_oparg == 0xFF) {
    if (ctx->ctx_code->code.code->bc_size - ctx->ctx_ip < XEN_ULONG_SIZE) {
      ctx->ctx_error = 1;
      return (bc_Instruct_t){{NOP, 0}, {0}};
    }
    oparg = 0;
    for (Xen_size_t i = 0; i < XEN_ULONG_SIZE; i++) {
      bc_Instruct_t extend_arg_instr =
          ctx->ctx_code->code.code->bc_array[ctx->ctx_ip++];
      oparg |= ((Xen_ulong_t)extend_arg_instr.hdr.bci_oparg) << (8 * i);
    }
  }
  Dispatcher[instr.hdr.bci_opcode](vmr, ctx, oparg);
  return instr;
}

Xen_Instance* vm_run(Xen_size_t id) {
  VM_Run vmr = {id, NULL, 0};
#ifndef NDEBUG
  const char* previous_op = "No-OP";
  Xen_ssize_t previous_offset = -1;
#endif
  bc_Instruct_t previous_instruct = (bc_Instruct_t){{NOP, 0}, {0}};
  Xen_Source_Address* bt = NULL;
  Xen_size_t bt_count = 0;
  Xen_size_t bt_cap = 0;
  while (
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack) &&
      ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack))->ctx_id >=
          id &&
      !vmr.halt && !program.closed) {
    if (Xen_VM_Except_Active()) {
      while ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack) &&
             ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack))
                     ->ctx_id >= id) {
        if (bt_count >= bt_cap) {
          Xen_size_t new_cap = (bt_cap == 0) ? 4 : bt_cap * 2;
          bt = Xen_Realloc(bt, new_cap * sizeof(Xen_Source_Address));
          bt_cap = new_cap;
        }
        bt[bt_count++] =
            ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack))
                ->ctx_code->code.code
                ->bc_array[((RunContext_ptr)run_context_stack_peek_top(
                                &vm->vm_ctx_stack))
                               ->ctx_ip -
                           1]
                .sta;
        run_context_stack_pop_top(&vm->vm_ctx_stack);
      }
      goto except;
    }
    RunContext_ptr ctx =
        (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
    if (ctx->ctx_error) {
      run_context_stack_pop_top(&vm->vm_ctx_stack);
#ifndef NDEBUG
      printf("VM Error: opcode '%s'; offset %ld;\n", previous_op,
             previous_offset);
#endif
      while ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack) &&
             ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack))
                     ->ctx_id >= id) {
        run_context_stack_pop_top(&vm->vm_ctx_stack);
      }
      return NULL;
    }
    if (!ctx->ctx_running) {
      run_context_stack_pop_top(&vm->vm_ctx_stack);
      continue;
    }
    previous_instruct = vm_run_instruct(&vmr, (Xen_Instance*)ctx);
#ifndef NDEBUG
    previous_op = Instruct_Info_Table[previous_instruct.hdr.bci_opcode].name;
    previous_offset = ctx->ctx_ip - 1;
#endif
  }
  while (
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack) &&
      ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack))->ctx_id >=
          id) {
    run_context_stack_pop_top(&vm->vm_ctx_stack);
  }
  return vmr.retval;

except:
  Xen_VM_Except_Show(bt, bt_count);
  return NULL;
}

Xen_Instance* vm_run_top(void) {
  return vm_run(
      ((RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack))->ctx_id);
}
