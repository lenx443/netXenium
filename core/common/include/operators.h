#ifndef __OPERATORS_H__
#define __OPERATORS_H__

#include "instance.h"

typedef enum {
  Xen_EQ = 0,
  Xen_Assignment,
} Xen_Opr;

Xen_Instance *Xen_Operator_Eval_Pair(Xen_Instance *, Xen_Instance *, Xen_Opr);

#endif
