#ifndef __XEN_NIL_H__
#define __XEN_NIL_H__

#include "instance.h"

#define nil Xen_Nil()
#define if_nil_eval(value) if (Xen_Nil_Eval((Xen_Instance *)(value)))
#define if_nil_neval(value) if (Xen_Nil_NEval((Xen_Instance *)(value)))

Xen_Instance *Xen_Nil();
int Xen_Nil_Eval(Xen_Instance *);
int Xen_Nil_NEval(Xen_Instance *);

extern Xen_Instance Xen_Nil_Def;

#endif
