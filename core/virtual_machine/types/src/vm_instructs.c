#include "vm_instructs.h"

// clang-format off
static STACK_EFFECT(nop_stack_effect, 0)
static STACK_EFFECT(push_stack_effect, 1)
static STACK_EFFECT(pop_stack_effect, -1)
static STACK_EFFECT(load_stack_effect, 1)
static STACK_EFFECT(load_prop_stack_effect, 1)
static STACK_EFFECT(load_index_stack_effect, -1)
static STACK_EFFECT(load_attr_stack_effect, 0)
static STACK_EFFECT(store_stack_effect, -1)
static STACK_EFFECT(store_prop_stack_effect, -1)
static STACK_EFFECT(store_index_stack_effect, -3)
static STACK_EFFECT(store_attr_stack_effect, -2)
static STACK_EFFECT(make_tuple_stack_effect, -oparg + 1)
static STACK_EFFECT(make_vector_stack_effect, -oparg + 1)
static STACK_EFFECT(make_vector_from_iterable_stack_effect, 0)
static STACK_EFFECT(make_map_stack_effect, -oparg * 2 + 1)
static STACK_EFFECT(make_function_stack_effect, -1)
static STACK_EFFECT(make_function_nargs_stack_effect, 1)
static STACK_EFFECT(call_stack_effect, -oparg)
static STACK_EFFECT(call_kw_stack_effect, -oparg - 1)
static STACK_EFFECT(binaryop_stack_effect, -1)
static STACK_EFFECT(unary_positive_stack_effect, 0)
static STACK_EFFECT(unary_negative_stack_effect, 0)
static STACK_EFFECT(unary_bit_not_stack_effect, 0)
static STACK_EFFECT(unary_not_stack_effect, 0)
static STACK_EFFECT(copy_stack_effect, 1)
static STACK_EFFECT(print_top_stack_effect, 0)
static STACK_EFFECT(throw_stack_effect, -1)
static STACK_EFFECT(jump_stack_effect, 0)
static STACK_EFFECT(jump_if_true_stack_effect, -1)
static STACK_EFFECT(jump_if_false_stack_effect, -1)
static STACK_EFFECT(iter_get_stack_effect, 0)
static STACK_EFFECT(iter_for_stack_effect, 0)
static STACK_EFFECT(list_unpack_stack_effect, oparg - 1)
static STACK_EFFECT(list_unpack_start_stack_effect, oparg - 1)
static STACK_EFFECT(list_unpack_end_stack_effect, oparg - 1)
static STACK_EFFECT(catch_stack_push_stack_effect, 1)
static STACK_EFFECT(catch_stack_pop_stack_effect, 0)
static STACK_EFFECT(catch_stack_type_stack_effect, 0)
static STACK_EFFECT(build_implement_stack_effect, -1)
static STACK_EFFECT(build_implement_nbase_stack_effect, 0)
static STACK_EFFECT(return_stack_effect, 0)
static STACK_EFFECT(return_top_stack_effect, -1)
static STACK_EFFECT(return_build_implement_stack_effect, 0)

struct vm_Instruct_Info Instruct_Info_Table[HALT] = {
    [NOP] = {"", nop_stack_effect, 0},
    [PUSH] = {"PUSH", push_stack_effect, INSTRUCT_FLAG_CO_INSTANCE},
    [POP] = {"POP", pop_stack_effect, INSTRUCT_FLAG_ARG},
    [LOAD] = {"LOAD", load_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [LOAD_PROP] = {"LOAD_PROP", load_prop_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [LOAD_INDEX] = {"LOAD_INDEX", load_index_stack_effect, 0},
    [LOAD_ATTR] = {"LOAD_ATTR", load_attr_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [STORE] = {"STORE", store_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [STORE_PROP] = {"STORE_PROP", store_prop_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [STORE_INDEX] = {"STORE_INDEX", store_index_stack_effect, 0},
    [STORE_ATTR] = {"STORE_ATTR", store_attr_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [MAKE_TUPLE] = {"MAKE_TUPLE", make_tuple_stack_effect, INSTRUCT_FLAG_ARG},
    [MAKE_VECTOR] = {"MAKE_VECTOR", make_vector_stack_effect, INSTRUCT_FLAG_ARG},
    [MAKE_VECTOR_FROM_ITERABLE] = {"MAKE_VECTOR_FROM_ITERABLE", make_vector_from_iterable_stack_effect, 0},
    [MAKE_MAP] = {"MAKE_MAP", make_map_stack_effect, INSTRUCT_FLAG_ARG},
    [MAKE_FUNCTION] = {"MAKE_FUNCTION", make_function_stack_effect, INSTRUCT_FLAG_CO_CALLABLE},
    [MAKE_FUNCTION_NARGS] = {"MAKE_FUNCTION_NARGS", make_function_nargs_stack_effect, INSTRUCT_FLAG_CO_CALLABLE},
    [CALL] = {"CALL", call_stack_effect, INSTRUCT_FLAG_ARG},
    [CALL_KW] = {"CALL_KW", call_kw_stack_effect, INSTRUCT_FLAG_ARG},
    [BINARYOP] = {"BINARYOP", binaryop_stack_effect, INSTRUCT_FLAG_ARG},
    [UNARY_POSITIVE] = {"UNARY_POSITIVE", unary_positive_stack_effect, 0},
    [UNARY_NEGATIVE] = {"UNARY_NEGATIVE", unary_negative_stack_effect, 0},
    [UNARY_BIT_NOT] = {"UNARY_BIT_NOT", unary_bit_not_stack_effect, 0},
    [UNARY_NOT] = {"UNARY_NOT", unary_not_stack_effect, 0},
    [COPY] = {"COPY", copy_stack_effect, 0},
    [PRINT_TOP] = {"PRINT_TOP", print_top_stack_effect, 0},
    [THROW] = {"THROW", throw_stack_effect, 0},
    [JUMP] = {"JUMP", jump_stack_effect, INSTRUCT_FLAG_ARG},
    [JUMP_IF_TRUE] = {"JUMP_IF_TRUE", jump_if_true_stack_effect, INSTRUCT_FLAG_ARG},
    [JUMP_IF_FALSE] = {"JUMP_IF_FALSE", jump_if_false_stack_effect, INSTRUCT_FLAG_ARG},
    [ITER_GET] = {"ITER_GET", iter_get_stack_effect, 0},
    [ITER_FOR] = {"ITER_FOR", iter_for_stack_effect, INSTRUCT_FLAG_ARG},
    [LIST_UNPACK] = {"LIST_UNPACK", list_unpack_stack_effect, INSTRUCT_FLAG_ARG},
    [LIST_UNPACK_START] = {"LIST_UNPACK_START", list_unpack_start_stack_effect, INSTRUCT_FLAG_ARG},
    [LIST_UNPACK_END] = {"LIST_UNPACK_END", list_unpack_end_stack_effect, INSTRUCT_FLAG_ARG},
    [CATCH_STACK_PUSH] = {"CATCH_STACK_PUSH", catch_stack_push_stack_effect, INSTRUCT_FLAG_ARG},
    [CATCH_STACK_POP] = {"CATCH_STACK_POP", catch_stack_pop_stack_effect, 0},
    [CATCH_STACK_TYPE] = {"CATCH_STACK_TYPE", catch_stack_type_stack_effect, INSTRUCT_FLAG_CO_INSTANCE},
    [BUILD_IMPLEMENT] = {"BUILD_IMPLEMENT", build_implement_stack_effect, INSTRUCT_FLAG_CO_CALLABLE},
    [BUILD_IMPLEMENT_NBASE] = {"BUILD_IMPLEMENT_NBASE", build_implement_nbase_stack_effect, INSTRUCT_FLAG_CO_CALLABLE},
    [RETURN] = {"RETURN", return_stack_effect, INSTRUCT_FLAG_CO_INSTANCE},
    [RETURN_TOP] = {"RETURN_TOP", return_top_stack_effect, 0},
    [RETURN_BUILD_IMPLEMENT] = {"RETURN_BUILD_IMPLEMENT", return_build_implement_stack_effect, 0},
};
// clang-format on
