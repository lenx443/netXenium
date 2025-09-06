#ifndef __XEN_BOOLEAN_INSTANCE_H__
#define __XEN_BOOLEAN_INSTANCE_H__

#include <stdint.h>

#include "instance.h"

struct Xen_Boolean_Instance {
  Xen_INSTANCE_HEAD;
  uint8_t value;
};

typedef struct Xen_Boolean_Instance Xen_Boolean;

#endif
