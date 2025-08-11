#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include "implement.h"
#include <stddef.h>

#define Xen_INSTANCE struct __Implement *__impl;

struct __Instance {
  Xen_INSTANCE
};

struct __Instance *__instance_new(struct __Implement *);
void __instance_free(struct __Instance *);

#endif
