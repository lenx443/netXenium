#ifndef __AST_COMPILER_H__
#define __AST_COMPILER_H__

#include "block_list.h"
#include "instance.h"

int ast_compile(block_list_ptr, block_node_ptr *, Xen_Instance *);

#endif
