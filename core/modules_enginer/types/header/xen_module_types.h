#ifndef __XEN_MODULE_TYPES_H__
#define __XEN_MODULE_TYPES_H__

#include "callable.h"
#include "implement.h"

struct Xen_Module_Function {
  char* fun_name;
  Xen_Native_Func fun_func;
};

struct Xen_Module_Def {
  Xen_c_string_t mod_name;
  Xen_Native_Func mod_init;
  struct Xen_Module_Function* mod_functions;
  Xen_ImplementStruct** mod_implements;
};

typedef struct Xen_Module_Function Xen_Module_Function_Table[];
typedef struct Xen_Module_Def* Xen_Module_Def_Table[];

#endif
