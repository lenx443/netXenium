#ifndef __XEN_REGISTER_STREAM_H__
#define __XEN_REGISTER_STREAM_H__

#include "instance.h"
#include <stdbool.h>

typedef int (*Xen_RegisterSetHandle)(const char *, Xen_INSTANCE *);
typedef Xen_INSTANCE *(*Xen_RegisterGetHandle)(const char *);

struct Xen_RegisterStream {
  const char *prefix;
  bool exact_match;
  Xen_RegisterSetHandle set_handle;
  Xen_RegisterGetHandle get_handle;
};

#endif
