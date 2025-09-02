#ifndef __XEN_MODULE_H__
#define __XEN_MODULE_H__

#include "instance.h"
#include "instances_map.h"

Xen_Instance *Xen_Module_New(struct __Instances_Map *, Xen_Instance *);

#endif
