#ifndef __VM_INSTURCTS_H__
#define __VM_INSTURCTS_H__

#include <stddef.h>

#define STACK_EFFECT(fname, ret)                                               \
  size_t fname(size_t oparg) {                                                 \
    (void)oparg;                                                               \
    return ret;                                                                \
  }

enum vm_Instruct {
  PUSH = 0,
  POP,
  LOAD,
  LOAD_PROP,
  CALL,
  HALT,
};

extern size_t (*Instruct_Stack_Effect_Table[HALT])(size_t);

#endif
