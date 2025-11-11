#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <stdint.h>

#include "callable.h"
#include "instance.h"
#include "program_code.h"

#define Xen_COMPILE_PROGRAM 0
#define Xen_COMPILE_REPL 1

CALLABLE_ptr compiler(const char*, uint8_t);
int ast_compile(ProgramCode_t*, uint8_t, Xen_Instance*);
void program_stack_depth(ProgramCode_t*);

#endif
