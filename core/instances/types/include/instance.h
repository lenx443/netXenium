#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "gc_header.h"

typedef uint8_t Xen_Instance_Flag;

#define Xen_INSTANCE_HEAD                                                      \
  struct __GC_Header __gc;                                                     \
  struct __Implement* __impl;                                                  \
  size_t __size;                                                               \
  Xen_Instance_Flag __flags

#define Xen_INSTANCE_MAPPED_HEAD                                               \
  Xen_INSTANCE_HEAD;                                                           \
  struct __Instance* __map

#define Xen_INSTANCE struct __Instance

#define Xen_INSTANCE_MAPPED struct __Instance_Mapped

#define Xen_INSTANCE_SET(impl, flags) .__impl = impl, .__flags = (flags)

#define XEN_INSTANCE_FLAG_STATIC (1 << 0)
#define XEN_INSTANCE_FLAG_MAPPED (1 << 1)

#define XEN_INSTANCE_GET_FLAG(inst, flag)                                      \
  (((((struct __Instance*)inst)->__flags) & (flag)) != 0)

Xen_INSTANCE {
  Xen_INSTANCE_HEAD;
};
Xen_INSTANCE_MAPPED {
  Xen_INSTANCE_MAPPED_HEAD;
};

Xen_INSTANCE* Xen_Instance_Alloc(struct __Implement*);
Xen_INSTANCE* __instance_new(struct __Implement*, Xen_INSTANCE*, Xen_INSTANCE*,
                             Xen_Instance_Flag);
void __instance_free(Xen_GCHeader**);

typedef Xen_INSTANCE Xen_Instance;
typedef Xen_INSTANCE_MAPPED Xen_Instance_Mapped;

static inline struct __Implement* Xen_IMPL(void* inst) {
  return (((struct __Instance*)inst)->__impl);
}

Xen_size_t Xen_SIZE(void*);

#endif
