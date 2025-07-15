#ifndef __AST_COMPILER_H__
#define __AST_COMPILER_H__

#include "ast.h"
#include "block_list.h"

int ast_compile(block_list_ptr, block_node_ptr *, AST_Node_t **, size_t);

#endif
