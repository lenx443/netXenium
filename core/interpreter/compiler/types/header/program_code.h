#ifndef __PROGRAM_CODE_H__
#define __PROGRAM_CODE_H__

#include "bytecode.h"
#include "vm_consts.h"

struct ProgramCode {
  vm_Consts_ptr consts;
  Bytecode_Array_ptr code;
  size_t stack_depth;
};

typedef struct ProgramCode ProgramCode_t;

#endif
