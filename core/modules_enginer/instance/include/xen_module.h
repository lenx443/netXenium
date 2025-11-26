#ifndef __XEN_MODULE_H__
#define __XEN_MODULE_H__

#include "instance.h"
#include "xen_module_types.h"

Xen_Instance* Xen_Module_New(Xen_Instance*);

Xen_Instance* Xen_Module_From_Def(struct Xen_Module_Def);

#endif
