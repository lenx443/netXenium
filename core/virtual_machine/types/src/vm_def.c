#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "gc_header.h"
#include "instance.h"
#include "program.h"
#include "run_ctx_stack.h"
#include "vm_backtrace.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_except.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_string.h"
#include "xen_vector.h"

static VM_ptr vm = NULL;

static void InterruptHandler(int sign) {
  (void)sign;
  Xen_GC_Stop();
  Xen_Interrupt();
  Xen_GC_Start();
}

static void vm_def_trace(Xen_GCHeader* h) {
  VM* _vm = (VM*)h;
  Xen_GC_Trace_GCHeader(_vm->args);
  Xen_GC_Trace_GCHeader(_vm->modules);
  Xen_GC_Trace_GCHeader(_vm->modules_stack);
  Xen_GC_Trace_GCHeader(_vm->globals_instances);
  Xen_GC_Trace_GCHeader(_vm->globals_props);
  Xen_GC_Trace_GCHeader(_vm->paths_modules);
  Xen_GC_Trace_GCHeader(_vm->config);
  if (vm->except.active) {
    Xen_GC_Trace_GCHeader(vm->except.except);
  }
}

static void vm_def_destroy(Xen_GCHeader* h) {
  run_context_stack_free(&((VM_ptr)h)->vm_ctx_stack);
  Xen_Dealloc(h);
}

static int vm_load_modules_paths(void) {
  Xen_IGC_WRITE_FIELD(vm, vm->args, Xen_Vector_New());
  for (int i = 0; i < program.argc; i++) {
    Xen_Instance* arg = Xen_String_From_CString(program.argv[i]);
    Xen_Vector_Push((Xen_Instance*)vm->args->ptr, arg);
  }
  Xen_IGC_WRITE_FIELD(vm, vm->paths_modules, Xen_Vector_New());
  Xen_Instance* default_module_path =
      Xen_String_From_CString(XEN_INSTALL_PREFIX "/lib/netxenium");
  Xen_Vector_Push((Xen_Instance*)vm->paths_modules->ptr, default_module_path);
  return 1;
}

static int vm_load_config(void) {
  Xen_IGC_WRITE_FIELD(vm, vm->config, Xen_Map_New());
  if (!Xen_Map_Push_Pair_Str(
          (Xen_Instance*)vm->config->ptr,
          (Xen_Map_Pair_Str){"paths_modules",
                             (Xen_Instance*)vm->paths_modules->ptr})) {
    return 0;
  }
  return 1;
}

bool vm_create(void) {
  if (vm != NULL)
    return 1;
  vm = (VM_ptr)Xen_GC_New(sizeof(VM), vm_def_trace, vm_def_destroy);
  vm->ctx_id_count = 0;
  vm->vm_ctx_stack = NULL;
  vm->args = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->modules = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->modules_stack = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->globals_instances = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->globals_props = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->paths_modules = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->config = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->except.except = Xen_GCHandle_New((Xen_GCHeader*)vm);
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
  Xen_IGC_WRITE_FIELD(vm, vm->modules, Xen_Map_New());
  if (!vm->modules) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  Xen_IGC_WRITE_FIELD(vm, vm->modules_stack, Xen_Vector_New());
  if (!vm->modules_stack) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  Xen_IGC_WRITE_FIELD(vm, vm->globals_instances, Xen_Map_New());
  if (!vm->globals_instances) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  Xen_IGC_WRITE_FIELD(vm, vm->globals_props, Xen_Map_New());
  if (!vm->globals_props) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  char path_current[1024];
  if (!getcwd(path_current, 1024)) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  vm->path_current = Xen_CString_Dup(path_current);
  if (!vm_load_modules_paths()) {
    run_context_stack_free(&vm->vm_ctx_stack);
    return 0;
  }
  if (!vm_load_config()) {
    run_context_stack_free(&vm->vm_ctx_stack);
    Xen_Dealloc((void*)vm->path_current);
    return 0;
  }
  vm->except.active = 0;
  vm->except.bt = vm_backtrace_new();
  Xen_GC_Push_Root((Xen_GCHeader*)vm);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = InterruptHandler;
  sigaction(SIGINT, &sa, NULL);
  xen_globals->vm = &vm;
  return 1;
}

void vm_destroy(void) {
  if (!vm)
    return;
  Xen_Dealloc((void*)vm->path_current);
  Xen_GCHandle_Free(vm->args);
  Xen_GCHandle_Free(vm->modules);
  Xen_GCHandle_Free(vm->modules_stack);
  Xen_GCHandle_Free(vm->globals_instances);
  Xen_GCHandle_Free(vm->globals_props);
  Xen_GCHandle_Free(vm->paths_modules);
  Xen_GCHandle_Free(vm->config);
  Xen_GCHandle_Free(vm->except.except);
  vm_backtrace_free(vm->except.bt);
  Xen_GC_Pop_Root();
}
