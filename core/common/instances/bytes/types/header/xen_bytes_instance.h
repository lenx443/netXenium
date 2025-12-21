#ifndef __XEN_BYTES_INSTANCE_H__
#define __XEN_BYTES_INSTANCE_H__

#include "instance.h"
#include "xen_typedefs.h"

struct Xen_Bytes_Instance {
  Xen_INSTANCE_HEAD;
  Xen_uint8_t* bytes;
  Xen_size_t capacity;
};

typedef struct Xen_Bytes_Instance Xen_Bytes;

#endif
