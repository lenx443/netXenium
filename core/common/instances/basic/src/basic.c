#include "basic.h"
#include "attrs.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "run_ctx_stack.h"
#include "vm_def.h"
#include "xen_alloc.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_life.h"
#include "xen_map.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_tuple.h"

static void basic_trace(Xen_GCHeader* h) {
  struct __Implement* impl = (struct __Implement*)h;
  if (impl->__props && impl->__props->ptr &&
      Xen_Nil_NEval((Xen_Instance*)impl->__props->ptr)) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)impl->__props->ptr);
  }
  if (impl->__base && impl->__base->ptr &&
      Xen_Nil_NEval((Xen_Instance*)impl->__base->ptr)) {
    Xen_GC_Trace_GCHeader((Xen_GCHeader*)impl->__base->ptr);
  }
}

static Xen_Instance* basic_create(struct __Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct __Implement* impl =
      (struct __Implement*)Xen_Instance_Alloc(&Xen_Basic);
  if (!impl) {
    return NULL;
  }
  impl->__impl_name = NULL;
  impl->__inst_trace = NULL;
  impl->__props = Xen_GCHandle_New();
  impl->__base = Xen_GCHandle_New();
  impl->__inst_size = sizeof(struct __Instance);
  impl->__create = NULL;
  impl->__destroy = NULL;
  impl->__string = NULL;
  impl->__raw = NULL;
  impl->__callable = NULL;
  impl->__hash = NULL;
  impl->__get_attr = NULL;
  impl->__set_attr = NULL;
  return nil;
}

static Xen_Instance* basic_destroy(struct __Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct __Implement* impl = (struct __Implement*)self;
  if (!impl)
    return NULL;
  if (impl->__impl_name)
    Xen_Dealloc(impl->__impl_name);
  if (impl->__props) {
    Xen_GCHandle_Free(impl->__props);
  }
  if (impl->__base) {
    Xen_GCHandle_Free(impl->__base);
  }
  return nil;
}

static Xen_Instance* basic_callable(struct __Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  struct __Implement* impl = (struct __Implement*)self;
  Xen_Instance* inst = __instance_new(impl, args, kwargs, 0);
  if (!inst) {
    return NULL;
  }
  Xen_IGC_Push(inst);
  if (!Xen_Attr_Create(inst, args, kwargs)) {
    Xen_IGC_Pop();
    return NULL;
  }
  Xen_IGC_Pop();
  vm_stack_push(((RunContext_ptr)run_context_stack_peek_top(
                     &(*xen_globals->vm)->vm_ctx_stack))
                    ->ctx_stack,
                inst);
  return nil;
}

static Xen_Instance* basic_string(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<Basic>");
  if (!string) {
    return NULL;
  }
  return string;
}

static Xen_Instance* basic_get_attr(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  struct __Implement* impl = (struct __Implement*)self;
  if (impl->__props == NULL || impl->__props->ptr ||
      Xen_Nil_Eval((Xen_Instance*)impl->__props->ptr) ||
      Xen_IMPL(impl->__props->ptr) != xen_globals->implements->map) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != xen_globals->implements->string) {
    return NULL;
  }
  Xen_IGC_Push(key);
  Xen_Instance* attr = Xen_Map_Get((Xen_Instance*)impl->__props->ptr, key);
  if (!attr) {
    Xen_IGC_Pop();
    return NULL;
  }
  Xen_IGC_Pop();
  return attr;
}

struct __Implement Xen_Basic = {
    Xen_INSTANCE_SET(&Xen_Basic, 0),
    .__impl_name = "Basic",
    .__inst_size = sizeof(struct __Implement),
    .__inst_default_flags = 0x00,
    .__inst_trace = basic_trace,
    .__props = NULL,
    .__alloc = NULL,
    .__create = basic_create,
    .__destroy = basic_destroy,
    .__string = basic_string,
    .__raw = basic_string,
    .__callable = basic_callable,
    .__hash = NULL,
    .__get_attr = basic_get_attr,
    .__set_attr = NULL,
};
