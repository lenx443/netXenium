#ifndef __XEN_MODULE_INSTANCE_H__
#define __XEN_MODULE_INSTANCE_H__

#include "instance.h"
#include "instances_map.h"

struct Xen_Module_Instance {
  Xen_INSTANCE_HEAD;
  Xen_Instance *mod_map;
  Xen_Instance *mod_context;
};

typedef struct Xen_Module_Instance Xen_Module;

#endif
