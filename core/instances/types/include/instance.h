#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t Xen_Instance_Flag;

#define Xen_TYPE(inst) (((struct __Instance *)inst)->__impl)

#ifndef XEN_DEBUG_REFERS
#define Xen_ADD_REF(inst)                                                                \
  ((inst) ? (((struct __Instance *)(inst))->__refers++, (inst)) : (inst))

#define Xen_DEL_REF(inst)                                                                \
  do {                                                                                   \
    if (inst && ((struct __Instance *)inst)->__refers > 0) {                             \
      if (--(((struct __Instance *)inst)->__refers) == 0)                                \
        __instance_free((struct __Instance *)(inst));                                    \
    }                                                                                    \
  } while (0)
#else
#define Xen_ADD_REF(inst)                                                                \
  ((inst) ? (((struct __Instance *)(inst))->__refers++,                                  \
             printf("[ADD_REF] %s=%p ref=%zd at %s:%d\n", #inst, inst,                   \
                    ((struct __Instance *)(inst))->__refers, __FILE__, __LINE__),        \
             (inst))                                                                     \
          : (inst))

#define Xen_DEL_REF(inst)                                                                \
  do {                                                                                   \
    if (inst) {                                                                          \
      printf("[DEL_REF] %s=%p ref=%zd at %s:%d\n", #inst, inst,                          \
             ((struct __Instance *)(inst))->__refers, __FILE__, __LINE__);               \
      ((struct __Instance *)(inst))->__refers--;                                         \
      if (((struct __Instance *)(inst))->__refers <= 0)                                  \
        __instance_free((struct __Instance *)(inst));                                    \
    }                                                                                    \
  } while (0)
#endif

#define Xen_INSTANCE_HEAD                                                                \
  size_t __refers;                                                                       \
  struct __Implement *__impl;                                                            \
  Xen_Instance_Flag __flags;

#define Xen_INSTANCE_MAPPED_HEAD                                                         \
  Xen_INSTANCE_HEAD;                                                                     \
  struct __Instance *__map;

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

Xen_INSTANCE *__instance_new(struct __Implement *, Xen_INSTANCE *, Xen_Instance_Flag);
void __instance_free(Xen_INSTANCE *);

typedef Xen_INSTANCE Xen_Instance;
typedef Xen_INSTANCE_MAPPED Xen_Instance_Mapped;

#endif
