#ifndef __XEN_NUMBER_INSTANCE_H__
#define __XEN_NUMBER_INSTANCE_H__

#include <stddef.h>
#include <stdint.h>

#include "instance.h"

struct Xen_Number_Instance {
  Xen_INSTANCE_HEAD;
  uint32_t *digits;
  size_t size;
  int8_t sign;
};

typedef struct Xen_Number_Instance Xen_Number;

#endif
