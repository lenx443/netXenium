#include "vm.h"
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "gc_header.h"
#include "instance.h"
#include "program.h"
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
#include "xen_nil.h"
#include "xen_tuple.h"

static void InterruptHandler(int sign) {
  (void)sign;
  Xen_GC_Stop();
  Xen_Interrupt();
  Xen_GC_Start();
}

static void vm_def_trace(Xen_GCHeader* h) {
  VM* _vm = (VM*)h;
  Xen_GC_Trace_GCHeader(_vm->current_ctx);
  Xen_GC_Trace_GCHeader(_vm->args);
  Xen_GC_Trace_GCHeader(_vm->modules);
  Xen_GC_Trace_GCHeader(_vm->modules_stack);
  Xen_GC_Trace_GCHeader(_vm->globals_instances);
  Xen_GC_Trace_GCHeader(_vm->globals_props);
  Xen_GC_Trace_GCHeader(_vm->paths_modules);
  Xen_GC_Trace_GCHeader(_vm->config);
  if (_vm->except.active) {
    Xen_GC_Trace_GCHeader(_vm->except.except);
  }
  if (_vm->evloop.active) {
    if (_vm->evloop.evloop->ptr)
      Xen_GC_Trace_GCHeader(_vm->evloop.evloop);
  }
}

static void vm_def_destroy(Xen_GCHeader* h) {
  Xen_Dealloc(h);
}

static int vm_load_modules_paths(void) {
  VM* vm = Xen_VM();
  Xen_IGC_WRITE_FIELD(vm->paths_modules, Xen_Vector_New());
  Xen_Instance* default_module_path =
      Xen_String_From_CString(XEN_INSTALL_PREFIX "/lib/netxenium");
  Xen_Vector_Push((Xen_Instance*)vm->paths_modules->ptr, default_module_path);
  return 1;
}

static int vm_load_config(void) {
  VM* vm = Xen_VM();
  Xen_IGC_WRITE_FIELD(vm->config, Xen_Map_New());
  if (!Xen_Map_Push_Pair_Str(
          (Xen_Instance*)vm->config->ptr,
          (Xen_Map_Pair_Str){"paths_modules",
                             (Xen_Instance*)vm->paths_modules->ptr})) {
    return 0;
  }
  return 1;
}

bool vm_create(void) {
  VM* vm = (VM_ptr)Xen_GC_New(sizeof(VM), vm_def_trace, vm_def_destroy);
  xen_globals->program->vm = vm;
  vm->current_ctx = Xen_GCHandle_New_From((Xen_GCHeader*)vm, (Xen_GCHeader*)nil);
  vm->args = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->modules = Xen_GCHandle_New_From((Xen_GCHeader*)vm, (Xen_GCHeader*)Xen_Map_New());
  vm->modules_stack = Xen_GCHandle_New_From((Xen_GCHeader*)vm, (Xen_GCHeader*)Xen_Vector_New());
  vm->globals_instances = Xen_GCHandle_New_From((Xen_GCHeader*)vm, (Xen_GCHeader*)Xen_Map_New());
  vm->globals_props = Xen_GCHandle_New_From((Xen_GCHeader*)vm, (Xen_GCHeader*)Xen_Map_New());
  vm->paths_modules = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->config = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->except.except = Xen_GCHandle_New((Xen_GCHeader*)vm);
  vm->evloop.evloop = Xen_GCHandle_New((Xen_GCHeader*)vm);
  Xen_Instance** args_array = Xen_Alloc(xen_globals->program->argc * sizeof(Xen_Instance*));
  if (!args_array) {
    return 0;
  }
  for (int i = 0; i < xen_globals->program->argc; i++) {
    Xen_INSTANCE* arg_value = Xen_String_From_CString(xen_globals->program->argv[i]);
    if (!arg_value) {
      Xen_Dealloc(args_array);
      return 0;
    }
    args_array[i] = arg_value;
  }
  Xen_IGC_WRITE_FIELD(vm->args, Xen_Tuple_From_Array(xen_globals->program->argc, args_array));
  Xen_Dealloc(args_array);
  char path_current[1024];
  if (!getcwd(path_current, 1024)) {
    return 0;
  }
  vm->path_current = Xen_CString_Dup(path_current);
  if (!vm_load_modules_paths()) {
    return 0;
  }
  if (!vm_load_config()) {
    Xen_Dealloc((void*)vm->path_current);
    return 0;
  }
  vm->except.active = 0;
  vm->except.bt = vm_backtrace_new();
  vm->evloop.active = 0;
  Xen_GC_Push_Root((Xen_GCHeader*)vm);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = InterruptHandler;
  sigaction(SIGINT, &sa, NULL);
  return 1;
}

void vm_destroy(void) {
  VM* vm = Xen_VM();
  Xen_Dealloc((void*)vm->path_current);
  Xen_GCHandle_Free(vm->args);
  Xen_GCHandle_Free(vm->modules);
  Xen_GCHandle_Free(vm->modules_stack);
  Xen_GCHandle_Free(vm->globals_props);
  Xen_GCHandle_Free(vm->paths_modules);
  Xen_GCHandle_Free(vm->config);
  Xen_GCHandle_Free(vm->except.except);
  Xen_GCHandle_Free(vm->evloop.evloop);
  vm_backtrace_free(vm->except.bt);
  Xen_GC_Pop_Root();
}
