#ifndef __XEN_REGISTER_H__
#define __XEN_REGISTER_H__

#include "instance.h"

int xen_register_prop_set(const char*, Xen_Instance*);
Xen_Instance* xen_register_prop_get(const char*);

#endif
