#ifndef __XEN_REGISTER_FLOW_H__
#define __XEN_REGISTER_FLOW_H__

#include "instance.h"
#include <stdbool.h>

typedef Xen_INSTANCE *(*Xen_RegisterGetHandle)(const char *);

struct Xen_RegisterFlow {
  const char *prefix;
  bool exact_match;
  Xen_RegisterGetHandle get_handle;
};

#endif
