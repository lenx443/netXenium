#include "vm_instructs.h"

static STACK_EFFECT(nop_stack_effect, 0);
static STACK_EFFECT(push_stack_effect, 1);
static STACK_EFFECT(pop_stack_effect, -1);
static STACK_EFFECT(load_stack_effect, 1);
static STACK_EFFECT(load_prop_stack_effect, 1);
static STACK_EFFECT(load_index_stack_effect, -1);
static STACK_EFFECT(load_attr_stack_effect, 0);
static STACK_EFFECT(store_stack_effect, -1);
static STACK_EFFECT(store_prop_stack_effect, -1);
static STACK_EFFECT(store_index_stack_effect, -3);
static STACK_EFFECT(store_attr_stack_effect, -2);
static STACK_EFFECT(call_stack_effect, -oparg);
static STACK_EFFECT(call_kw_stack_effect, -oparg - 1);
static STACK_EFFECT(binaryop_stack_effect, -1);
static STACK_EFFECT(unary_positive_stack_effect, 0);
static STACK_EFFECT(unary_negative_stack_effect, 0);
static STACK_EFFECT(unary_not_stack_effect, 0);
static STACK_EFFECT(copy_stack_effect, 1);
static STACK_EFFECT(print_top_stack_effect, 0);
static STACK_EFFECT(jump_stack_effect, 0);
static STACK_EFFECT(jump_if_true_stack_effect, -1);
static STACK_EFFECT(jump_if_false_stack_effect, -1);

struct vm_Instruct_Info Instruct_Info_Table[HALT] = {
    [NOP] = {"", nop_stack_effect, 0},
    [PUSH] = {"PUSH", push_stack_effect, INSTRUCT_FLAG_CO_INSTANCE},
    [POP] = {"POP", pop_stack_effect, 0},
    [LOAD] = {"LOAD", load_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [LOAD_PROP] = {"LOAD_PROP", load_prop_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [LOAD_INDEX] = {"LOAD_INDEX", load_index_stack_effect, 0},
    [LOAD_ATTR] = {"LOAD_ATTR", load_attr_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [STORE] = {"STORE", store_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [STORE_PROP] = {"STORE_PROP", store_prop_stack_effect,
                    INSTRUCT_FLAG_CO_NAME},
    [STORE_INDEX] = {"STORE_INDEX", store_index_stack_effect, 0},
    [STORE_ATTR] = {"STORE_ATTR", store_attr_stack_effect,
                    INSTRUCT_FLAG_CO_NAME},
    [CALL] = {"CALL", call_stack_effect, INSTRUCT_FLAG_ARG},
    [CALL_KW] = {"CALL_KW", call_kw_stack_effect, INSTRUCT_FLAG_ARG},
    [BINARYOP] = {"BINARYOP", binaryop_stack_effect, INSTRUCT_FLAG_ARG},
    [UNARY_POSITIVE] = {"UNARY_POSITIVE", unary_positive_stack_effect, 0},
    [UNARY_NEGATIVE] = {"UNARY_NEGATIVE", unary_negative_stack_effect, 0},
    [UNARY_NOT] = {"UNARY_NOT", unary_not_stack_effect, 0},
    [COPY] = {"COPY", copy_stack_effect, 0},
    [PRINT_TOP] = {"PRINT_TOP", print_top_stack_effect, 0},
    [JUMP] = {"JUMP", jump_stack_effect, INSTRUCT_FLAG_ARG},
    [JUMP_IF_TRUE] = {"JUMP_IF_TRUE", jump_if_true_stack_effect,
                      INSTRUCT_FLAG_ARG},
    [JUMP_IF_FALSE] = {"JUMP_IF_FALSE", jump_if_false_stack_effect,
                       INSTRUCT_FLAG_ARG},
};
