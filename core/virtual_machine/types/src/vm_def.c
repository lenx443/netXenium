#include <signal.h>
#include <stdbool.h>
#include <string.h>

#include "gc_header.h"
#include "instance.h"
#include "logs.h"
#include "program.h"
#include "run_ctx_stack.h"
#include "vm_backtrace.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_except.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_string.h"
#include "xen_vector.h"

#define error(msg, ...) log_add(NULL, ERROR, "VM", msg, ##__VA_ARGS__)

static void InterruptHandler(int sign) {
  (void)sign;
  Xen_Interrupt();
}

static void vm_def_trace([[maybe_unused]] Xen_GCHeader* h) {
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)vm->modules);
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)vm->modules_stack);
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)vm->globals_instances);
  Xen_GC_Trace_GCHeader((Xen_GCHeader*)vm->globals_props);
  if (vm->except.active) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)vm->except.except);
  }
}

static void vm_def_destroy(Xen_GCHeader** h) {
  run_context_stack_free(&((VM_ptr)*h)->vm_ctx_stack);
  Xen_Dealloc(*h);
}

bool vm_create(void) {
  if (vm != NULL)
    return 1;
  vm = (VM_ptr)Xen_GC_New(sizeof(VM), vm_def_trace, vm_def_destroy);
  if (!vm) {
    error("No hay memoria disponible");
    return 0;
  }
  vm->ctx_id_count = 0;
  vm->vm_ctx_stack = NULL;
  Xen_Instance** args_array = Xen_Alloc(program.argc * sizeof(Xen_Instance*));
  if (!args_array) {
    return 0;
  }
  for (int i = 0; i < program.argc; i++) {
    Xen_INSTANCE* arg_value = Xen_String_From_CString(program.argv[i]);
    if (!arg_value) {
      Xen_Dealloc(args_array);
      return 0;
    }
    args_array[i] = arg_value;
  }
  Xen_Dealloc(args_array);
  vm->modules = Xen_Map_New();
  if (!vm->modules) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  vm->modules_stack = Xen_Vector_New();
  if (!vm->modules_stack) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  vm->globals_instances = Xen_Map_New();
  if (!vm->globals_instances) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  vm->globals_props = Xen_Map_New();
  if (!vm->globals_props) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  vm->except.active = 0;
  vm->except.except = NULL;
  vm->except.bt = vm_backtrace_new();
  Xen_GC_Push_Root((Xen_GCHeader*)vm);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = InterruptHandler;
  sigaction(SIGINT, &sa, NULL);
  return 1;
}

void vm_destroy(void) {
  if (!vm)
    return;
  Xen_GC_Pop_Root();
}

VM_ptr vm = NULL;
