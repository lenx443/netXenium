#include <stddef.h>

#include "vm_instructs.h"

static STACK_EFFECT(push_stack_effect, 1);
static STACK_EFFECT(pop_stack_effect, -1);
static STACK_EFFECT(load_stack_effect, 1);
static STACK_EFFECT(load_prop_stack_effect, 1);
static STACK_EFFECT(call_stack_effect, -oparg);
static STACK_EFFECT(binaryop_stack_effect, -1);
static STACK_EFFECT(attr_get_stack_effect, 0);
static STACK_EFFECT(unary_positive_stack_effect, 0);
static STACK_EFFECT(unary_negative_stack_effect, 0);
static STACK_EFFECT(unary_not_stack_effect, 0);

struct vm_Instruct_Info Instruct_Info_Table[HALT] = {
    [PUSH] = {"PUSH", push_stack_effect, INSTRUCT_FLAG_CO_INSTANCE},
    [POP] = {"POP", pop_stack_effect, 0},
    [LOAD] = {"LOAD", load_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [LOAD_PROP] = {"LOAD_PROP", load_prop_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [CALL] = {"CALL", call_stack_effect, INSTRUCT_FLAG_ARG},
    [BINARYOP] = {"BINARYOP", binaryop_stack_effect, INSTRUCT_FLAG_ARG},
    [ATTR_GET] = {"ATTR_GET", attr_get_stack_effect, INSTRUCT_FLAG_CO_NAME},
    [UNARY_POSITIVE] = {"UNARY_POSITIVE", unary_positive_stack_effect, 0},
    [UNARY_NEGATIVE] = {"UNARY_NEGATIVE", unary_negative_stack_effect, 0},
    [UNARY_NOT] = {"UNARY_NOT", unary_not_stack_effect, 0},
};
