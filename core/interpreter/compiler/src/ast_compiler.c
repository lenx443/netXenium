#include <stddef.h>

#include "ast_compiler.h"
#include "block_list.h"
#include "instance.h"

#define error(msg, ...) log_add(NULL, ERROR, "AST Compiler", msg, ##__VA_ARGS__)
#define info(msg, ...) log_add(NULL, INFO, "AST Compiler", msg, ##__VA_ARGS__)

int ast_compile(block_list_ptr block_result, block_node_ptr* block,
                Xen_Instance* ast) {
  if (!ast)
    return 0;
  return 0;
}
