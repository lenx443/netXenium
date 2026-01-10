#ifndef __COMPILER_EXT_H__
#define __COMPILER_EXT_H__

#include "block_list.h"
#include "callable.h"

int blocks_linealizer(block_list_ptr);
CALLABLE_ptr blocks_compiler(block_list_ptr);

#endif
