#include "basic.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx.h"
#include "vm.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"

static void basic_trace(Xen_GCHeader* h) {
  struct __Implement* impl = (struct __Implement*)h;
  if_nil_neval(impl->__props) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)impl->__props);
  }
}

static Xen_Instance* basic_create(ctx_id_t id, struct __Instance* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct __Implement* impl = (struct __Implement*)self;
  impl->__impl_name = NULL;
  impl->__props = Xen_Map_New();
  if (!impl->__props) {
    return NULL;
  }
  impl->__inst_size = sizeof(struct __Instance);
  impl->__create = NULL;
  impl->__destroy = NULL;
  impl->__callable = NULL;
  impl->__hash = NULL;
  impl->__get_attr = NULL;
  return nil;
}

static Xen_Instance* basic_destroy(ctx_id_t id, struct __Instance* self,
                                   Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct __Implement* impl = (struct __Implement*)self;
  if (!impl)
    return NULL;
  if (impl->__impl_name)
    Xen_Dealloc(impl->__impl_name);
  return nil;
}

static Xen_Instance* basic_callable(ctx_id_t id, struct __Instance* self,
                                    Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct __Implement* impl = (struct __Implement*)self;
  Xen_Instance* inst = __instance_new(impl, args, kwargs, 0);
  if (!inst) {
    return NULL;
  }
  if (impl->__create) {
    Xen_Instance* rsult =
        Xen_VM_Call_Native_Function(impl->__create, inst, args, kwargs);
    if (!rsult) {
      return NULL;
    }
  }
  return inst;
}

static Xen_Instance* basic_string(ctx_id_t id, Xen_Instance* self,
                                  Xen_Instance* args, Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<Basic>");
  if (!string) {
    return NULL;
  }
  return string;
}

struct __Implement Xen_Basic = {
    Xen_INSTANCE_SET(&Xen_Basic, 0),
    .__impl_name = "Basic",
    .__inst_size = sizeof(struct __Implement),
    .__inst_default_flags = 0x00,
    .__inst_trace = basic_trace,
    .__props = &Xen_Nil_Def,
    .__create = basic_create,
    .__destroy = basic_destroy,
    .__string = basic_string,
    .__raw = basic_string,
    .__callable = basic_callable,
    .__hash = NULL,
};
