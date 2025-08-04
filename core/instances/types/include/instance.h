#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include "implement.h"
#include <stddef.h>

struct __Instance {
  char *__name;
  struct __Implement *__impl;
};

struct __Instance *__instance_new(char *, struct __Implement *);
void __instnace_free(struct __Instance *);

#endif
