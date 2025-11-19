#ifndef __COMPILER_EXT_H__
#define __COMPILER_EXT_H__

#include "block_list.h"
#include "program_code.h"

int blocks_linealizer(block_list_ptr);
int blocks_compiler(block_list_ptr, ProgramCode_t*);

#endif
