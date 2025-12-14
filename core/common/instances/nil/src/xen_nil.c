#include "xen_nil.h"
#include "instance.h"
#include "xen_life.h"

static Xen_Instance Xen_Nil_Def;

Xen_Instance* Xen_Nil_GetInstance(void) {
  Xen_Nil_Def.__impl = xen_globals->implements->nilp;
  Xen_Nil_Def.__flags = XEN_INSTANCE_FLAG_STATIC;
  return (Xen_Instance*)&Xen_Nil_Def;
}

Xen_Instance* Xen_Nil(void) {
  return xen_globals->nil_instance;
}

int Xen_Nil_Eval(Xen_Instance* value) {
  return (value == xen_globals->nil_instance ||
          value->__impl == xen_globals->implements->nilp);
}

int Xen_Nil_NEval(Xen_Instance* value) {
  return (value != xen_globals->nil_instance &&
          value->__impl != xen_globals->implements->nilp);
}
