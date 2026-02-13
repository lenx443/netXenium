#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include <stdint.h>

#include "instance.h"
#include "vm_scope.h"
#include "xen_typedefs.h"

Xen_Instance* interpreter(Xen_c_string_t, const char*, uint8_t, Xen_Instance *, Xen_Instance*, Xen_VM_Scopes*);
Xen_Instance* Xen_Eval(Xen_c_string_t);

#endif
