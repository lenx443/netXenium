#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include <stddef.h>
#include <stdint.h>

#include "call_args.h"

typedef uint8_t Xen_Instance_Flag;

#define Xen_TYPE(inst) (((struct __Instance *)inst)->__impl)

#define Xen_ADD_REF(inst)                                                                \
  do {                                                                                   \
    if (inst) ((struct __Instance *)inst)->__refers++;                                   \
  } while (0)

#define Xen_DEL_REF(inst)                                                                \
  do {                                                                                   \
    if (inst && ((struct __Instance *)inst)->__refers > 0) {                             \
      if (--(((struct __Instance *)inst)->__refers) == 0)                                \
        __instance_free((struct __Instance *)(inst));                                    \
    }                                                                                    \
  } while (0)

#define Xen_INSTANCE_HEAD                                                                \
  size_t __refers;                                                                       \
  struct __Implement *__impl;                                                            \
  Xen_Instance_Flag __flags;

#define Xen_INSTANCE_MAPPED_HEAD                                                         \
  Xen_INSTANCE_HEAD;                                                                     \
  struct __Instances_Map *__map;

#define Xen_INSTANCE struct __Instance

#define Xen_INSTANCE_MAPPED struct __Instance_Mapped

#define Xen_INSTANCE_SET(refers, impl, flags)                                            \
  .__refers = refers, .__impl = impl, .__flags = (flags)

#define XEN_INSTANCE_FLAG_STATIC (1 << 0)
#define XEN_INSTANCE_FLAG_MAPPED (1 << 1)

#define XEN_INSTANCE_GET_FLAG(inst, flag)                                                \
  (((((struct __Instance *)inst)->__flags) & (flag)) != 0)

Xen_INSTANCE{Xen_INSTANCE_HEAD};
Xen_INSTANCE_MAPPED{Xen_INSTANCE_MAPPED_HEAD};

Xen_INSTANCE *__instance_new(struct __Implement *, CallArgs *, Xen_Instance_Flag);
void __instance_free(Xen_INSTANCE *);

#endif
