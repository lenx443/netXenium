#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "block_list.h"
#include "callable.h"
#include "program_code.h"

CALLABLE_ptr compiler(const char*);
int blocks_compiler(ProgramCode_t*, block_list_ptr);
void blocks_linealizer(block_list_ptr);
void program_stack_depth(ProgramCode_t*);

#endif
