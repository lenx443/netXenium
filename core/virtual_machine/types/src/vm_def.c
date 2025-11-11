#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>

#include "instance.h"
#include "logs.h"
#include "program.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

bool vm_create() {
  if (vm != NULL)
    return 1;
  vm = malloc(sizeof(VM));
  if (!vm) {
    error("No hay memoria disponible");
    return 0;
  }
  vm->ctx_id_count = 0;
  vm->vm_ctx_stack = NULL;
  Xen_Instance** args_array = malloc(program.argc * sizeof(Xen_Instance*));
  if (!args_array) {
    free(vm);
    return 0;
  }
  for (int i = 0; i < program.argc; i++) {
    Xen_INSTANCE* arg_value = Xen_String_From_CString(program.argv[i]);
    if (!arg_value) {
      free(vm);
      free(args_array);
      return 0;
    }
    args_array[i] = arg_value;
  }
  Xen_Instance* args = Xen_Tuple_From_Array(program.argc, args_array);
  if (!args) {
    free(vm);
    for (int i = 0; i < program.argc; i++) {
      Xen_DEL_REF(args_array[i]);
    }
    free(args_array);
    return 0;
  }
  for (int i = 0; i < program.argc; i++) {
    Xen_DEL_REF(args_array[i]);
  }
  free(args_array);
  if (!run_context_stack_push(&vm->vm_ctx_stack, nil, nil, nil, args, nil)) {
    free(vm);
    Xen_DEL_REF(args);
    return 0;
  }
  Xen_DEL_REF(args);
  vm->root_context =
      (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  vm->modules_contexts = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if (!vm->modules_contexts) {
    run_context_stack_free(&vm->vm_ctx_stack);
    free(vm);
  }
  vm->global_props = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if (!vm->global_props) {
    Xen_DEL_REF(vm->modules_contexts);
    run_context_stack_free(&vm->vm_ctx_stack);
    free(vm);
  }
  return 1;
}

void vm_destroy() {
  if (!vm)
    return;
  Xen_DEL_REF(vm->modules_contexts);
  Xen_DEL_REF(vm->global_props);
  run_context_stack_free(&vm->vm_ctx_stack);
  free(vm);
}

VM_ptr vm = NULL;
