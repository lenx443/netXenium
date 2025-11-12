#include "xen_function_implement.h"
#include "basic.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_function_instance.h"
#include "xen_nil.h"
#include "xen_string.h"

static Xen_Instance* function_create(ctx_id_t id, struct __Instance* self,
                                     Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ptr inst = (Xen_Function_ptr)self;
  inst->fun_callable = NULL;
  inst->closure = nil;
  return nil;
}

static Xen_Instance* function_destroy(ctx_id_t id, struct __Instance* self,
                                      Xen_Instance* args,
                                      Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ptr inst = (Xen_Function_ptr)self;
  if (inst->fun_callable)
    callable_free(inst->fun_callable);
  if_nil_neval(inst->closure) Xen_DEL_REF(inst->closure);
  return nil;
}

static Xen_Instance* function_callable(ctx_id_t id, struct __Instance* self,
                                       Xen_Instance* args,
                                       Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Function_ptr inst = (Xen_Function_ptr)self;
  if (inst->fun_callable) {
    Xen_Instance* ret =
        vm_run_callable(inst->fun_callable, inst->closure, nil, args, kwargs);
    if (!ret) {
      return NULL;
    }
    return ret;
  }
  return NULL;
}

static Xen_Instance* function_string(ctx_id_t id, Xen_Instance* self,
                                     Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Instance* string = Xen_String_From_CString("<Function>");
  if (!string) {
    return NULL;
  }
  return string;
}

struct __Implement Xen_Function_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Function",
    .__inst_size = sizeof(Xen_Function),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__create = function_create,
    .__destroy = function_destroy,
    .__string = function_string,
    .__raw = function_string,
    .__callable = function_callable,
    .__hash = NULL,
};
