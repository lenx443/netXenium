#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include <stddef.h>
#include <stdint.h>

#define Xen_ADD_REF(inst)                                                                \
  do {                                                                                   \
    if (inst) (inst)->__refers++;                                                        \
  } while (0)

#define Xen_DEL_REF(inst)                                                                \
  do {                                                                                   \
    if (inst && (inst)->__refers > 0) {                                                  \
      if (--((inst)->__refers) == 0) __instance_free((struct __Instance *)(inst));       \
    }                                                                                    \
  } while (0)

#define Xen_INSTANCE_HEAD                                                                \
  size_t __refers;                                                                       \
  struct __Implement *__impl;                                                            \
  uint8_t __flags;

#define Xen_INSTANCE struct __Instance

#define Xen_INSTANCE_SET(refers, impl, flags)                                            \
  .__refers = refers, .__impl = impl, .__flags = flags

#define XEN_INSTANCE_FLAG_STATIC (1UL << 7)
#define XEN_INSTANCE_FLAG_IS_STATIC(flag) (flag >> 7)

Xen_INSTANCE{Xen_INSTANCE_HEAD};

Xen_INSTANCE *__instance_new(struct __Implement *);
void __instance_free(Xen_INSTANCE *);

#endif
