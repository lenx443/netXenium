#ifndef __VM_INSTURCTS_H__
#define __VM_INSTURCTS_H__

#include <stddef.h>

enum vm_Instruct {
  PUSH = 0,
  POP,
  HALT,
};

extern size_t Instruct_Stack_Effect_Table[HALT];

#endif
