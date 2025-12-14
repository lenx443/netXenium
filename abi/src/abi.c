#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "netXenium.h"
#include "xen_alloc.h"
#include "xen_cstrings.h"
#include "xen_module_types.h"
#include "xen_nil.h"
#include "xen_typedefs.h"

struct Xen_Module_Def* Xen_Module_Define(Xen_c_string_t name,
                                         Xen_Native_Func init,
                                         struct Xen_Module_Function* functions,
                                         Xen_ImplementStruct** implements) {
  struct Xen_Module_Def* module = Xen_Alloc(sizeof(struct Xen_Module_Def));
  module->mod_name = name;
  module->mod_init = init;
  module->mod_functions = functions;
  module->mod_implements = implements;
  return module;
}

void Xen_Debug_Print(Xen_c_string_t str, ...) {
#ifndef NDEBUG
  va_list args;
  va_start(args, str);
  vprintf(str, args);
  va_end(args);
#endif
}

Xen_Implement* Xen_Implement_Make(Xen_c_string_t name, Xen_size_t size) {
  Xen_Implement* impl =
      (Xen_Implement*)__instance_new(&Xen_Basic, Xen_Nil(), Xen_Nil(), 0);
  impl->__impl_name = Xen_CString_Dup(name);
  impl->__size = size;
  return impl;
}
