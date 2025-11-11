#include "xen_method_implement.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_function_instance.h"
#include "xen_method_instance.h"
#include "xen_nil.h"
#include "xen_string.h"

static Xen_Instance* method_alloc(ctx_id_t id, struct __Instance* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Method* method = (Xen_Method*)self;
  method->function = nil;
  method->self = nil;
  return nil;
}

static Xen_Instance* method_destroy(ctx_id_t id, struct __Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Method* method = (Xen_Method*)self;
  Xen_DEL_REF(method->function);
  Xen_DEL_REF(method->self);
  return nil;
}

static Xen_Instance* method_string(ctx_id_t id, struct __Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return Xen_String_From_CString("<Method>");
}

static Xen_Instance* method_callable(ctx_id_t id, struct __Instance* self,
                                     Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Method* method = (Xen_Method*)self;
  Xen_Function_ptr function = (Xen_Function_ptr)method->function;
  return vm_run_callable(function->fun_callable, function->closure,
                         method->self, args, kwargs);
}

Xen_Implement Xen_Method_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Method",
    .__inst_size = sizeof(struct Xen_Method_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = method_alloc,
    .__destroy = method_destroy,
    .__string = method_string,
    .__raw = method_string,
    .__callable = method_callable,
    .__hash = NULL,
    .__get_attr = Xen_Basic_Get_Attr_Static,
};
