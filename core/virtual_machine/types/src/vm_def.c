#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>

#include "instance.h"
#include "logs.h"
#include "program.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_vector.h"
#include "xen_vector_implement.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

bool vm_create() {
  if (vm != NULL) return 1;
  vm = malloc(sizeof(VM));
  if (!vm) {
    error("No hay memoria disponible");
    return 0;
  }
  vm->ctx_id_count = 0;
  vm->vm_ctx_stack = NULL;
  Xen_Instance *args = __instance_new(&Xen_Vector_Implement, nil, 0);
  if_nil_eval(args) {
    free(vm);
    return 0;
  }
  for (int i = 0; i < program.argc; i++) {
    Xen_INSTANCE *arg_value = Xen_String_From_CString(program.argv[i]);
    if_nil_eval(arg_value) {
      free(vm);
      Xen_DEL_REF(args);
    }
    if (!Xen_Vector_Push(args, arg_value)) {
      Xen_DEL_REF(arg_value);
      free(vm);
      Xen_DEL_REF(args);
      return 0;
    }
  }
  if (!run_context_stack_push(&vm->vm_ctx_stack, nil, nil, nil, args)) {
    free(vm);
    Xen_DEL_REF(args);
    return 0;
  }
  Xen_DEL_REF(args);
  vm->root_context = (RunContext_ptr)run_context_stack_peek_top(&vm->vm_ctx_stack);
  vm->modules_contexts = Xen_Map_New(XEN_MAP_DEFAULT_CAP);
  if_nil_eval(vm->modules_contexts) {
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
  if (!vm) return;
  Xen_DEL_REF(vm->modules_contexts);
  Xen_DEL_REF(vm->global_props);
  run_context_stack_free(&vm->vm_ctx_stack);
  free(vm);
}

VM_ptr vm = NULL;
