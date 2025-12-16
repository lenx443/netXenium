#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include "xen_typedefs.h"

struct __Implement;

#define Xen_INSTANCE_HEAD                                                      \
  XEN_GC_HEAD                                                                  \
  struct __Implement* __impl;                                                  \
  Xen_size_t __size;                                                           \
  Xen_Instance_Flag __flags

#define Xen_INSTANCE_MAPPED_HEAD                                               \
  Xen_INSTANCE_HEAD;                                                           \
  struct __Instance* __map

#define XEN_INSTANCE_FLAG_STATIC (1 << 0)
#define XEN_INSTANCE_FLAG_MAPPED (1 << 1)

#define Xen_INSTANCE_SET(impl, flags) .__impl = impl, .__flags = (flags)

typedef Xen_uint8_t Xen_Instance_Flag;
typedef struct __Instance Xen_Instance;

Xen_Instance* Xen_Instance_Alloc(struct __Implement*);
Xen_Instance* __instance_new(struct __Implement*, Xen_Instance*, Xen_Instance*,
                             Xen_Instance_Flag);

Xen_size_t Xen_SIZE(void*);

#endif
