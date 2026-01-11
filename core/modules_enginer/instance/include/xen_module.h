#ifndef __XEN_MODULE_H__
#define __XEN_MODULE_H__

#include "instance.h"
#include "xen_module_types.h"
#include "xen_typedefs.h"

#define XEN_MODULE_GUEST 1
#define XEN_MODULE_NATIVE 2

Xen_Instance* Xen_Module_New(void);

Xen_Instance* Xen_Module_From_Def(struct Xen_Module_Def, Xen_c_string_t);

Xen_Instance* Xen_Module_Load(Xen_c_string_t, Xen_c_string_t, Xen_c_string_t,
                              Xen_Instance*, Xen_uint8_t);

Xen_Instance* Xen_Load(Xen_c_string_t);

#endif
