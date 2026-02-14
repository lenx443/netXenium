#include "m_async.h"
#include "callable.h"
#include "coroutine_instance.h"
#include "instance.h"
#include "netxenium/xen_function.h"
#include "run_ctx.h"
#include "vm_run.h"
#include "xen_function.h"
#include "xen_life.h"
#include "xen_module_types.h"

static Xen_Instance*
fn_run(Xen_Instance* self, Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ArgSpec args_def[] = {
      {"coro", XEN_FUNCTION_ARG_KIND_POSITIONAL, XEN_FUNCTION_ARG_IMPL_ANY, XEN_FUNCTION_ARG_REQUIRED, NULL},
      {NULL, XEN_FUNCTION_ARG_KIND_END, 0, 0, NULL},
  };

  Xen_Function_ArgBinding* binding =
      Xen_Function_ArgsParse(args, kwargs, args_def);
  if (!binding) {
    return NULL;
  }
  Xen_Instance* coro_inst = Xen_Function_ArgBinding_Search(binding, "coro")->value;
  Xen_Function_ArgBinding_Free(binding);
  if (Xen_IMPL(coro_inst) != xen_globals->implements->coroutine) {
    return NULL;
  }
  Xen_Coroutine* coro = (Xen_Coroutine*)coro_inst;
  Xen_Instance* ctx = (Xen_Instance*)coro->context->ptr;
  Xen_Ctx_Enable_End(ctx);
  return vm_run(ctx);
}

Xen_Module_Function_Table functions = {
  {"run", fn_run},
  {NULL, NULL},
};

struct Xen_Module_Def Module_Async = {
    .mod_name = "Async",
    .mod_init = NULL,
    .mod_functions = functions,
    .mod_implements = NULL,
};
