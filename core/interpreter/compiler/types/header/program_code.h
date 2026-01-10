#ifndef __PROGRAM_CODE_H__
#define __PROGRAM_CODE_H__

#include "bytecode.h"
#include "netxenium/gc_header.h"

struct ProgramCode {
  Xen_GCHandle* consts;
  Bytecode_Array_ptr code;
  Xen_size_t stack_depth;
};

typedef struct ProgramCode ProgramCode_t;

#endif
