#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <stdint.h>

#include "block_list.h"
#include "callable.h"
#include "instance.h"

#define Xen_COMPILE_PROGRAM 0
#define Xen_COMPILE_REPL 1
#define Xen_COMPILE_FUNCTION 2

CALLABLE_ptr compiler(const char*, uint8_t);
CALLABLE_ptr compiler_function(Xen_Instance*);
int ast_compile(block_list_ptr, block_node_ptr*, uint8_t, Xen_Instance*);

#endif
