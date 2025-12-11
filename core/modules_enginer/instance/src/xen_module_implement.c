#include "xen_module_implement.h"
#include "basic.h"
#include "basic_templates.h"
#include "callable.h"
#include "implement.h"
#include "instance.h"
#include "xen_alloc.h"
#include "xen_igc.h"
#include "xen_map.h"
#include "xen_module_instance.h"
#include "xen_nil.h"
#include "xen_string.h"
#include "xen_string_implement.h"
#include "xen_tuple.h"

static Xen_Instance* module_alloc(Xen_Instance* self, Xen_Instance* args,
                                  Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Module* module = (Xen_Module*)Xen_Instance_Alloc(&Xen_Module_Implement);
  if (!module) {
    return NULL;
  }
  return (Xen_Instance*)module;
}

static Xen_Instance* module_destroy(Xen_Instance* self, Xen_Instance* args,
                                    Xen_Instance* kwargs) {
  NATIVE_CLEAR_ARG_NEVER_USE;
  Xen_Module* module = (Xen_Module*)self;
  Xen_Dealloc((void*)module->mod_name);
  Xen_Dealloc((void*)module->mod_path);
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
  NATIVE_CLEAR_ARG_NEVER_USE;
  if (Xen_SIZE(args) != 1) {
    return NULL;
  }
  Xen_Instance_Mapped* mapped = (Xen_Instance_Mapped*)self;
  Xen_Instance* key = Xen_Tuple_Get_Index(args, 0);
  if (Xen_IMPL(key) != &Xen_String_Implement) {
    return NULL;
  }
  Xen_IGC_Push(key);
  Xen_Instance* attr = Xen_Map_Get(mapped->__map, key);
  if (!attr) {
    return NULL;
  }
  Xen_IGC_Pop();
  return attr;
}

struct __Implement Xen_Module_Implement = {
    Xen_INSTANCE_SET(&Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Module",
    .__inst_size = sizeof(struct Xen_Module_Instance),
    .__inst_default_flags = XEN_INSTANCE_FLAG_MAPPED,
    .__inst_trace = Xen_Basic_Mapped_Trace,
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
