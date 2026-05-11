#ifndef __XEN_MODULE_TYPES_H__
#define __XEN_MODULE_TYPES_H__

#include "callable.h"
#include "implement.h"
#include "xen_function_instance.h"
#include "xen_typedefs.h"

struct Xen_Module_Function {
  char* fun_name;
  Xen_Native_Func fun_func;
};

struct Xen_Module_Function_Async {
  char* fun_name;
  Xen_Native_Func_Async fun_func;
  Xen_size_t fun_data_size;
};

struct Xen_Module_Def {
  Xen_c_string_t mod_name;
  Xen_Native_Func mod_init;
  struct Xen_Module_Function* mod_functions;
  struct Xen_Module_Function_Async* mod_functions_async;
  Xen_ImplementStruct** mod_implements;
};

typedef struct Xen_Module_Function Xen_Module_Function_Table[];
typedef struct Xen_Module_Function_Async Xen_Module_Function_Async_Table[];
typedef struct Xen_Module_Def* Xen_Module_Def_Table[];

#endif
