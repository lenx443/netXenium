#ifndef __XEN_COMMAND_H__
#define __XEN_COMMAND_H__

#include "callable.h"
#include "instance.h"
#include "program_code.h"
#include "xen_command_instance.h"

Xen_Command *Xen_Command_From_Native(Xen_Native_Func, Xen_INSTANCE *);
Xen_Command *Xen_Command_From_Program(ProgramCode_t, Xen_INSTANCE *);

#endif
