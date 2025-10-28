#ifndef __OPERATORS_H__
#define __OPERATORS_H__

#include "instance.h"

typedef enum {
  Xen_OPR_EQ = 0,
  Xen_OPR_GET_INDEX,
  Xen_OPR_POW,
  Xen_OPR_MUL,
  Xen_OPR_DIV,
  Xen_OPR_END,
} Xen_Opr;

Xen_Instance* Xen_Operator_Eval_Pair(Xen_Instance*, Xen_Instance*, Xen_Opr);
Xen_Instance* Xen_Operator_Eval_Pair_Steal2(Xen_Instance*, Xen_Instance*,
                                            Xen_Opr);

#endif
