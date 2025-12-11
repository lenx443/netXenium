#ifndef __XEN_MODULE_INSTANCE_H__
#define __XEN_MODULE_INSTANCE_H__

#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Module_Instance {
  Xen_INSTANCE_MAPPED_HEAD;
  Xen_bool_t mod_initializing;
  Xen_bool_t mod_initialized;
  Xen_c_string_t mod_name;
  Xen_c_string_t mod_path;
};

typedef struct Xen_Module_Instance Xen_Module;

#endif
