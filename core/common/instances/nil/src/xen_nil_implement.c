#include "xen_nil_implement.h"
#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "xen_nil.h"

struct __Implement Xen_Nil_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Nil",
    .__inst_size = sizeof(Xen_Instance),
    .__inst_default_flags = 0x00,
    .__props = &Xen_Nil_Def,
    .__alloc = NULL,
    .__destroy = NULL,
    .__string = NULL,
    .__raw = NULL,
    .__callable = NULL,
    .__hash = NULL,
};
