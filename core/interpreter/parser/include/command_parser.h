#ifndef __COMMAND_PARSER_H__
#define __COMMAND_PARSER_H__

#include "instance.h"
#include "vm_scope.h"

char** Xen_Command_Parser(const char*, int*, int*, Xen_Instance*, Xen_Instance*, Xen_Instance*, Xen_VM_Scopes*);

#endif
