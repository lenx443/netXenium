#ifndef __XEN_STRING_INSTANCE_H__
#define __XEN_STRING_INSTANCE_H__

#include <stddef.h>

#include "instance.h"

struct Xen_String_Instance {
  Xen_INSTANCE_MAPPED_HEAD;
  char *characters;
};

typedef struct Xen_String_Instance Xen_String;

#endif
