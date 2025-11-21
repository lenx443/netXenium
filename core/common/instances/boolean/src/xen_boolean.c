#include "xen_boolean.h"
#include "instance.h"
#include "xen_boolean_implement.h"
#include "xen_boolean_instance.h"

Xen_Boolean Xen_True_Instance = {
    Xen_INSTANCE_SET(&Xen_Boolean_Implement, XEN_INSTANCE_FLAG_STATIC),
    .value = 1,
};

Xen_Boolean Xen_False_Instance = {
    Xen_INSTANCE_SET(&Xen_Boolean_Implement, XEN_INSTANCE_FLAG_STATIC),
    .value = 0,
};
