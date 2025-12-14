#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "netXenium.h"
#include "xen_alloc.h"
#include "xen_module_types.h"

struct Xen_Module_Def*
Xen_Module_Define(Xen_c_string_t name, struct Xen_Module_Function* functions) {
  struct Xen_Module_Def* module = Xen_Alloc(sizeof(struct Xen_Module_Def));
  module->mod_name = name;
  module->mod_functions = functions;
  module->mod_init = NULL;
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
