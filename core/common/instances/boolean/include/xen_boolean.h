#ifndef __XEN_BOOLEAN_H__
#define __XEN_BOOLEAN_H__

#include "instance.h"

Xen_Instance* Xen_True_GetInstance(void);
Xen_Instance* Xen_False_GetInstance(void);

#define Xen_True (xen_globals->true_instance)
#define Xen_False (xen_globals->false_instance)

#endif
