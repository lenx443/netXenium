#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "callable.h"
#include "instance.h"
#include "program_code.h"

CALLABLE_ptr compiler(const char*);
int ast_compile(ProgramCode_t*, Xen_Instance*);
void program_stack_depth(ProgramCode_t*);

#endif
