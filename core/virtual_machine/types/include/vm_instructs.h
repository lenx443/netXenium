#ifndef __VM_INSTURCTS_H__
#define __VM_INSTURCTS_H__

#include <stddef.h>
#include <stdint.h>

#define STACK_EFFECT(fname, ret)                                               \
  size_t fname(size_t oparg) {                                                 \
    (void)oparg;                                                               \
    return ret;                                                                \
  }

#define INSTRUCT_FLAG_CO_NAME (1 << 0)
#define INSTRUCT_FLAG_CO_INSTANCE (1 << 1)
#define INSTRUCT_FLAG_ARG (1 << 2)

enum vm_Instruct {
  PUSH = 0,
  POP,
  LOAD,
  LOAD_PROP,
  CALL,
  BINARYOP,
  INDEX_GET,
  ATTR_GET,
  UNARY_POSITIVE,
  UNARY_NEGATIVE,
  UNARY_NOT,
  HALT,
};

struct vm_Instruct_Info {
  const char* name;
  size_t (*stack_effect)(size_t);
  uint8_t flags;
};

extern struct vm_Instruct_Info Instruct_Info_Table[HALT];

#endif
