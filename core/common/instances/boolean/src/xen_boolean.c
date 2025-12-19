#include "xen_boolean.h"
#include "instance.h"
#include "xen_boolean_instance.h"
#include "xen_life.h"

static Xen_Boolean Xen_True_Instance;
static Xen_Boolean Xen_False_Instance;

Xen_Instance* Xen_True_GetInstance(void) {
  Xen_True_Instance.__impl = xen_globals->implements->boolean;
  Xen_True_Instance.__flags = XEN_INSTANCE_FLAG_STATIC;
  Xen_True_Instance.value = 1;
  return (Xen_Instance*)&Xen_True_Instance;
}

Xen_Instance* Xen_False_GetInstance(void) {
  Xen_False_Instance.__impl = xen_globals->implements->boolean;
  Xen_False_Instance.__flags = XEN_INSTANCE_FLAG_STATIC;
  Xen_False_Instance.value = 0;
  return (Xen_Instance*)&Xen_False_Instance;
}

Xen_bool_t Xen_IsBoolean(Xen_Instance* v) {
  return Xen_IMPL(v) == xen_globals->implements->boolean;
}
