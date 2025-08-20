#ifndef __XEN_STRING_INSTANCE_H__
#define __XEN_STRING_INSTANCE_H__

#include <stddef.h>

#include "instance.h"

struct Xen_String_Instance {
  Xen_INSTANCE_HEAD;
  char *characters;
  size_t length;
  size_t capacity;
};

typedef struct Xen_String_Instance Xen_String;

#endif
