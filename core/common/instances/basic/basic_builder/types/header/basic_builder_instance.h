#ifndef __BASIC_BUILDER_INSTANCE_H__
#define __BASIC_BUILDER_INSTANCE_H__

#include "implement.h"
#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Basic_Builder_Instance {
  Xen_INSTANCE_MAPPED_HEAD;
  Xen_c_string_t name;
  Xen_Implement* base;
};

typedef struct Xen_Basic_Builder_Instance Xen_Basic_Builder;

#endif
