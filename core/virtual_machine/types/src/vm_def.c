#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>

#include "call_args.h"
#include "instances_map.h"
#include "logs.h"
#include "program.h"
#include "run_ctx_stack.h"
#include "vm_def.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

bool vm_create() {
  if (vm != NULL) return 1;
  vm = malloc(sizeof(VM));
  if (!vm) {
    error("No hay memoria disponible");
    return 0;
  }
  vm->vm_ctx_stack = NULL;
  CallArgs *args = call_args_new();
  if (!args) {
    free(vm);
    return 0;
  }
  for (int i = 0; i < program.argc; i++) {
    if (!call_args_push(args, (struct CArg){
                                  CARG_POINTER,
                                  program.argv[i],
                                  strlen(program.argv[i]),
                              })) {
      free(vm);
      call_args_free(args);
      return 0;
    }
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack, NULL, NULL, args)) {
    free(vm);
    call_args_free(args);
    return 0;
  }
  vm->root_context = run_context_stack_pop_top(&vm->vm_ctx_stack);
  vm->global_props = __instances_map_new(INSTANCES_MAP_DEFAULT_CAPACITY);
  if (!vm->global_props) {
    run_context_stack_free(&vm->vm_ctx_stack);
    free(vm);
  }
  return 1;
}

void vm_destroy() {
  if (!vm) return;
  __instances_map_free(vm->global_props);
  run_context_stack_free(&vm->vm_ctx_stack);
  free(vm);
}

VM_ptr vm = NULL;
