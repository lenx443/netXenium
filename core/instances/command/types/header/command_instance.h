#ifndef __COMMAND_INSTANCE_H__
#define __COMMAND_INSTANCE_H__

#include "callable.h"
#include "instance.h"

struct Xen_Command_Instance {
  Xen_INSTANCE_HEAD;
  CALLABLE_ptr cmd_callable;
};

typedef struct Xen_Command_Instance Xen_Command;
typedef Xen_Command *Xen_Command_ptr;

#endif
