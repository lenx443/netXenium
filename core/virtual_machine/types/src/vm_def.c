#include <malloc.h>
#include <stdlib.h>

#include "logs.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "vm_def.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

int vm_create() {
  if (vm != NULL) return 1;
  vm = malloc(sizeof(VM));
  if (!vm) {
    error("No hay memoria disponible");
    return 0;
  }
  vm->vm_ctx_stack = NULL;
  if (!run_context_stack_push(&vm->vm_ctx_stack)) {
    free(vm);
    return 0;
  }
  Main_Context = run_context_stack_pop_top(&vm->vm_ctx_stack);
  return 1;
}

void vm_destroy() {
  if (!vm) return;
  run_context_stack_free(&vm->vm_ctx_stack);
  free(vm);
}

VM_ptr vm = NULL;
RunContext_ptr Main_Context = NULL;
