#include <stdint.h>
#include <stdio.h>

#include "callable.h"
#include "compiler.h"
#include "instance.h"
#include "interpreter.h"
#include "run_ctx.h"
#include "run_ctx_stack.h"
#include "vm.h"
#include "vm_def.h"
#include "vm_run.h"
#include "xen_nil.h"
#include "xen_typedefs.h"

Xen_Instance* interpreter(Xen_c_string_t file_name, const char* file_content,
                          uint8_t compile_mode) {
  if (!file_name || !file_content) {
    return NULL;
  }
  CALLABLE_ptr code = compiler(file_name, file_content, compile_mode);
  if (Xen_VM_Except_Active()) {
    Xen_VM_Except_Backtrace_Show();
    return NULL;
  }
  if (!code) {
    return NULL;
  }
#ifndef NDEBUG
  printf("== Running ==\n");
#endif
  Xen_Instance* ctx_inst = Xen_Ctx_New(
      nil, nil, nil, nil, nil,
      (Xen_Instance*)(*xen_globals->vm)->globals_instances->ptr, code);
  if (!ctx_inst) {
    return NULL;
  }
  if (!run_context_stack_push(&(*xen_globals->vm)->vm_ctx_stack, ctx_inst)) {
    return NULL;
  }
  Xen_Instance* retval = vm_run_top();
  if (Xen_VM_Except_Active()) {
    Xen_VM_Except_Backtrace_Show();
    return NULL;
  }
  if (!retval) {
    return NULL;
  }
  return retval;
}
