#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include <stddef.h>

#include "implement.h"

#define Xen_ADD_REF(inst)                                                                \
  do {                                                                                   \
    if (inst) (inst)->__refers++;                                                        \
  } while (0)

#define Xen_DEL_REF(inst)                                                                \
  do {                                                                                   \
    if (inst && --((inst)->__refers) == 0) __instance_free((struct __Instance *)inst);   \
  } while (0)

#define Xen_INSTANCE                                                                     \
  size_t __refers;                                                                       \
  struct __Implement *__impl;

struct __Instance {
  Xen_INSTANCE
};

struct __Instance *__instance_new(struct __Implement *);
void __instance_free(struct __Instance *);

#endif
