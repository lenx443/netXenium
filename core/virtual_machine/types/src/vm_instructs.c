#include <stddef.h>

#include "vm_instructs.h"

static STACK_EFFECT(push_stack_effect, 1);
static STACK_EFFECT(pop_stack_effect, -1);
static STACK_EFFECT(load_stack_effect, 1);
static STACK_EFFECT(load_prop_stack_effect, 1);
static STACK_EFFECT(call_stack_effect, -oparg);
static STACK_EFFECT(binaryop_stack_effect, -1);
static STACK_EFFECT(attr_get_stack_effect, 0);

size_t (*Instruct_Stack_Effect_Table[HALT])(size_t) = {
    [PUSH] = push_stack_effect,         [POP] = pop_stack_effect,
    [LOAD] = load_stack_effect,         [LOAD_PROP] = load_prop_stack_effect,
    [CALL] = call_stack_effect,         [BINARYOP] = binaryop_stack_effect,
    [ATTR_GET] = attr_get_stack_effect,
};
