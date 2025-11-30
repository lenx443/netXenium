#include "xen_nil.h"
#include "instance.h"
#include "xen_nil_implement.h"

Xen_Instance* Xen_Nil(void) {
  return &Xen_Nil_Def;
}

int Xen_Nil_Eval(Xen_Instance* value) {
  return (value == &Xen_Nil_Def || value->__impl == &Xen_Nil_Implement);
}

int Xen_Nil_NEval(Xen_Instance* value) {
  return (value != &Xen_Nil_Def && value->__impl != &Xen_Nil_Implement);
}

Xen_Instance Xen_Nil_Def = {
    Xen_INSTANCE_SET(&Xen_Nil_Implement, XEN_INSTANCE_FLAG_STATIC)};
