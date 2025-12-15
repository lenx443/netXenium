#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "implement.h"
#include "netxenium/netXenium.h"
#include "xen_alloc.h"
#include "xen_module_types.h"
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
