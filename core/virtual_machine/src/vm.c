#include <assert.h>
#include <stdio.h>

#include "callable.h"
#include "gc_header.h"
#include "instance.h"
#include "run_ctx_instance.h"
#include "source_file.h"
#include "vm.h"
#include "vm_backtrace.h"
#include "vm_def.h"
#include "vm_scope.h"
#include "xen_except_instance.h"
#include "xen_function.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_typedefs.h"

VM* Xen_VM(void) {
  return xen_globals->program->vm;
}

Xen_Instance* Xen_VM_Current_Ctx(void) {
  return ((Xen_Instance*)Xen_VM()->current_ctx->ptr);
}

void Xen_VM_Set_Current_Ctx(Xen_Instance* ctx) {
  if (Xen_VM()->current_ctx->ptr)
    ((RunContext_ptr)Xen_VM()->current_ctx->ptr)->ctx_running = 0;
  if (!ctx) {
    Xen_VM()->current_ctx->ptr = NULL;
    return;
  }
  Xen_GC_Write_Field(&Xen_VM()->current_ctx, (Xen_GCHeader*)ctx);
  ((RunContext_ptr)Xen_VM()->current_ctx->ptr)->ctx_running = 1;
}

bool Xen_VM_Store_Native_Function(Xen_Instance* inst_map, const char* name,
                                  Xen_Native_Func fun, Xen_Instance* closure) {
  Xen_INSTANCE* fun_inst = Xen_Function_From_Native(fun, closure);
  if (!fun_inst) {
    return false;
  }
  Xen_IGC_Push(fun_inst);
  if (!Xen_Map_Push_Pair_Str(inst_map, (Xen_Map_Pair_Str){name, fun_inst})) {
    Xen_IGC_Pop();
    return false;
  }
  Xen_IGC_Pop();
  return true;
}

bool Xen_VM_Store_Native_Function_Async(Xen_Instance* inst_map, const char* name, Xen_Native_Func_Async fun, Xen_size_t data_size) {
  Xen_INSTANCE* fun_inst = Xen_Function_From_Native_Async(fun, data_size);
  if (!fun_inst) {
    return false;
  }
  Xen_IGC_Push(fun_inst);
  if (!Xen_Map_Push_Pair_Str(inst_map, (Xen_Map_Pair_Str){name, fun_inst})) {
    Xen_IGC_Pop();
    return false;
  }
  Xen_IGC_Pop();
  return true;
}

Xen_Instance* Xen_VM_Call_Native_Function(Xen_Native_Func func,
                                          Xen_INSTANCE* self,
                                          Xen_Instance* args,
                                          Xen_Instance* kwargs) {
  Xen_Instance* ret = func(self, args, kwargs);
  if (!ret) {
    return NULL;
  }
  return ret;
}

Xen_INSTANCE* Xen_VM_Load_Instance(const char* name) {
  RunContext_ptr current_ctx = (RunContext_ptr)Xen_VM_Current_Ctx();
  while (current_ctx && Xen_Nil_NEval((Xen_Instance*)current_ctx)) {
    Xen_VM_Scope* current_scope =
        ((Xen_VM_Scopes*)current_ctx->ctx_scopes->ptr)->scopes;
    while (current_scope) {
      Xen_Instance* inst =
          Xen_Map_Get_Str((Xen_Instance*)current_scope->symbols->ptr, name);
      if (inst != NULL) {
        return inst;
      }
      current_scope = current_scope->next;
    }
    Xen_Instance* inst =
        Xen_Map_Get_Str((Xen_Instance*)current_ctx->ctx_instances->ptr, name);
    if (inst != NULL) {
      return inst;
    }
    current_ctx = (RunContext_ptr)current_ctx->ctx_closure->ptr;
  }
  Xen_Instance* inst = Xen_Map_Get_Str((Xen_Instance*)(Xen_VM())->globals_instances->ptr, name);
  if (inst != NULL) {
    return inst;
  }
  return NULL;
}

void Xen_VM_Ctx_Clear(RunContext_ptr ctx) {
  ctx->ctx_code = NULL;
  ctx->ctx_ip = 0;
  ctx->ctx_running = 0;
}

void Xen_VM_Except_Backtrace_Show(void) {
  Xen_Except* except = (Xen_Except*)Xen_VM()->except.except->ptr;
  puts("Unhandled exception occurred.");
  if (Xen_VM()->except.bt->bt_count > 0) {
    puts("BackTrace:");
  }
  for (Xen_size_t i = 0; i < Xen_VM()->except.bt->bt_count; i++) {
    printf("file: \"%s\"; line: %ld; column: %ld;\n",
           (*xen_globals->source_table)->st_files
           [(Xen_size_t)Xen_VM()->except.bt->bt_addrs[i].id]->sf_name,
           Xen_VM()->except.bt->bt_addrs[i].line,
           Xen_VM()->except.bt->bt_addrs[i].column);
  }
  if (except->message) {
    fputs(except->type, stdout);
    fputs(": ", stdout);
    puts(except->message);
  } else {
    fputs("Type: ", stdout);
    puts(except->type);
  }
  Xen_VM()->except.active = 0;
  vm_backtrace_clear(Xen_VM()->except.bt);
}

int Xen_VM_Except_Throw(Xen_Instance* except_inst) {
  assert(except_inst != NULL);
  if (Xen_VM()->except.active)
    return 1;
  Xen_IGC_Push(except_inst);
  Xen_Instance* except =
      Xen_Method_Attr_Str_Call(except_inst, "__except", nil, nil);
  if (!except) {
    Xen_IGC_Pop();
    return 0;
  }
  if (Xen_IMPL(except) != xen_globals->implements->except) {
    Xen_IGC_Pop();
    return 0;
  }
  Xen_VM()->except.active = 1;
  Xen_GC_Write_Field(&Xen_VM()->except.except, (Xen_GCHeader*)except);
  Xen_IGC_Pop();
  return 1;
}
