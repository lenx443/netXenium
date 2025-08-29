#include "basic.h"
#include "implement.h"
#include "instance.h"
#include "xen_nil_implement.h"

struct __Implement Xen_Nil_Implement = {
    Xen_INSTANCE_SET(0, &Xen_Basic, XEN_INSTANCE_FLAG_STATIC),
    .__impl_name = "Nil",
    .__inst_size = sizeof(Xen_Instance),
    .__inst_default_flags = 0x00,
    .__props = NULL,
    .__alloc = NULL,
    .__destroy = NULL,
    .__callable = NULL,
    .__hash = NULL,
};
