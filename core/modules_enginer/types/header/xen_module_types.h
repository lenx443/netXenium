#ifndef __XEN_MODULE_TYPES_H__
#define __XEN_MODULE_TYPES_H__

#include "callable.h"

struct Xen_Module_Command {
  char *cmd_name;
  Xen_Native_Func cmd_func;
};

struct Xen_Module_Def {
  char *mod_name;
  Xen_Native_Func mod_init;
  struct Xen_Module_Command *mod_commands;
};

typedef struct Xen_Module_Command Xen_Module_Command_Table[];
typedef struct Xen_Module_Def *Xen_Module_Def_Table[];

#endif
