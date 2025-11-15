#ifndef __VM_INSTURCTS_H__
#define __VM_INSTURCTS_H__

#include <stddef.h>
#include <stdint.h>

#include "xen_typedefs.h"

#define STACK_EFFECT(fname, ret)                                               \
  Xen_ssize_t fname(Xen_ssize_t oparg) {                                       \
    (void)oparg;                                                               \
    return ret;                                                                \
  }

#define INSTRUCT_FLAG_CO_NAME (1 << 0)
#define INSTRUCT_FLAG_CO_INSTANCE (1 << 1)
#define INSTRUCT_FLAG_ARG (1 << 2)

enum vm_Instruct {
  NOP = 0,
  PUSH,
  POP,
  LOAD,
  LOAD_PROP,
  LOAD_INDEX,
  LOAD_ATTR,
  STORE,
  STORE_PROP,
  STORE_INDEX,
  STORE_ATTR,
  MAKE_TUPLE,
  UNPACK_TUPLE,
  CALL,
  CALL_KW,
  BINARYOP,
  UNARY_POSITIVE,
  UNARY_NEGATIVE,
  UNARY_NOT,
  COPY,
  PRINT_TOP,
  JUMP,
  JUMP_IF_TRUE,
  JUMP_IF_FALSE,
  ITER_GET,
  ITER_FOR,
  HALT,
};

struct vm_Instruct_Info {
  const char* name;
  Xen_ssize_t (*stack_effect)(Xen_ssize_t);
  uint8_t flags;
};

extern struct vm_Instruct_Info Instruct_Info_Table[HALT];

#endif
