#ifndef __XEN_NUMBER_INSTANCE_H__
#define __XEN_NUMBER_INSTANCE_H__

#include <stddef.h>
#include <stdint.h>

#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Number_Instance {
  Xen_INSTANCE_HEAD;
  uint32_t* digits;
  Xen_size_t scale;
  Xen_size_t size;
  Xen_int8_t sign;
};

typedef struct Xen_Number_Instance Xen_Number;

#endif
