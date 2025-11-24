#include <stdbool.h>

#include "gc_header.h"
#include "instance.h"
#include "logs.h"
#include "program.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_string.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

bool vm_create() {
  if (vm != NULL)
    return 1;
  vm = Xen_Alloc(sizeof(VM));
  if (!vm) {
    error("No hay memoria disponible");
    return 0;
  }
  vm->ctx_id_count = 0;
  vm->vm_ctx_stack = NULL;
  Xen_Instance** args_array = Xen_Alloc(program.argc * sizeof(Xen_Instance*));
  if (!args_array) {
    Xen_Dealloc(vm);
    return 0;
  }
  for (int i = 0; i < program.argc; i++) {
    Xen_INSTANCE* arg_value = Xen_String_From_CString(program.argv[i]);
    if (!arg_value) {
      Xen_Dealloc(vm);
      Xen_Dealloc(args_array);
      return 0;
    }
    args_array[i] = arg_value;
  }
  Xen_Dealloc(args_array);
  vm->modules_contexts = Xen_Map_New();
  if (!vm->modules_contexts) {
    run_context_stack_free(&vm->vm_ctx_stack);
    Xen_Dealloc(vm);
  }
  Xen_GC_Push_Root((Xen_GCHeader*)vm->modules_contexts);
  vm->globals_instances = Xen_Map_New();
  if (!vm->globals_instances) {
    Xen_GC_Pop_Root();
    run_context_stack_free(&vm->vm_ctx_stack);
    Xen_Dealloc(vm);
    return 0;
  }
  Xen_GC_Push_Root((Xen_GCHeader*)vm->globals_instances);
  vm->global_props = Xen_Map_New();
  if (!vm->global_props) {
    Xen_GC_Pop_Roots(2);
    run_context_stack_free(&vm->vm_ctx_stack);
    Xen_Dealloc(vm);
    return 0;
  }
  Xen_GC_Push_Root((Xen_GCHeader*)vm->global_props);
  return 1;
}

void vm_destroy() {
  if (!vm)
    return;
  Xen_GC_Pop_Roots(3);
  run_context_stack_free(&vm->vm_ctx_stack);
  Xen_Dealloc(vm);
}

VM_ptr vm = NULL;
