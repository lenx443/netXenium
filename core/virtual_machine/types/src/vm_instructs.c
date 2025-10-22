#include <stddef.h>

#include "vm_instructs.h"

size_t Instruct_Stack_Effect_Table[HALT] = {[PUSH] = 1, [POP] = -1};
