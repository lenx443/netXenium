#ifndef __XEN_BOOLEAN_H__
#define __XEN_BOOLEAN_H__

#include "xen_boolean_instance.h"

extern Xen_Boolean Xen_True_Instance;
extern Xen_Boolean Xen_False_Instance;

#define Xen_True ((Xen_Instance *)&Xen_True_Instance)
#define Xen_False ((Xen_Instance *)&Xen_False_Instance)

#endif
