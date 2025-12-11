#ifndef __XEN_MODULE_H__
#define __XEN_MODULE_H__

#include "instance.h"
#include "xen_module_types.h"
#include "xen_typedefs.h"

Xen_Instance* Xen_Module_New(void);

Xen_Instance* Xen_Module_From_Def(struct Xen_Module_Def, Xen_c_string_t);

Xen_Instance* Xen_Module_Load(Xen_c_string_t, Xen_c_string_t, Xen_c_string_t,
                              Xen_Instance*);

#endif
