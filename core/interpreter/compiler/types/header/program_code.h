#ifndef __PROGRAM_CODE_H__
#define __PROGRAM_CODE_H__

#include "bytecode.h"
#include "vm_string_table.h"

struct ProgramCode {
  vm_String_Table_ptr strings;
  Bytecode_Array_ptr code;
};

typedef struct ProgramCode ProgramCode_t;

#endif
