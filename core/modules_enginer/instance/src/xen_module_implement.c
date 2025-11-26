#include "xen_module_implement.h"
#include "basic.h"
#include "callable.h"
#include "gc_header.h"
#include "implement.h"
#include "instance.h"
#include "xen_gc.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_map_implement.h"
#include "xen_module_instance.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_tuple.h"

static void module_trace(Xen_GCHeader* h) {
  Xen_Module* module = (Xen_Module*)h;
  if_nil_neval(module->mod_map)
      Xen_GC_Trace_GCHeader((Xen_GCHeader*)module->mod_map);
}

static Xen_Instance* module_alloc(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Module* module = (Xen_Module*)Xen_Instance_Alloc(&Xen_Module_Implement);
  if (!module) {
    return NULL;
  }
  module->mod_map = nil;
  return (Xen_Instance*)module;
}

static Xen_Instance* module_destroy(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  return nil;
}

static Xen_Instance* module_string(Xen_Instance* self, Xen_Instance* args,
                                   Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Instance* string = Xen_String_From_CString("<Module>");
  if (!string) {
    return NULL;
  }
  return string;
}

static Xen_Instance* module_get_attr(Xen_Instance* self, Xen_Instance* args,
                                     Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE
  Xen_Module* module = (Xen_Module*)self;
  if (module->mod_map == NULL || Xen_Nil_Eval(module->mod_map) ||
      Xen_IMPL(module->mod_map) != &Xen_Map_Implement) {
    return NULL;
  }
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != &Xen_String_Implement) {
    return NULL;
  }
  Xen_IGC_Push(key);
  Xen_Instance* attr = Xen_Map_Get(module->mod_map, key);
  if (!attr) {
    Xen_IGC_Pop();
    return NULL;
  }
  Xen_IGC_Pop();
  return attr;
}

struct __Implement Xen_Module_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Module",
    .__inst_size = sizeof(struct Xen_Module_Instance),
    .__inst_default_flags = 0x00,
    .__inst_trace = module_trace,
    .__props = &Xen_Nil_Def,
    .__alloc = module_alloc,
    .__create = NULL,
    .__destroy = module_destroy,
    .__string = module_string,
    .__raw = module_string,
    .__callable = NULL,
    .__hash = NULL,
    .__get_attr = module_get_attr,
};
