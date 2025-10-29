#ifndef __OPERATORS_H__
#define __OPERATORS_H__

#include "instance.h"

typedef enum {
  Xen_OPR_GET_INDEX,
  Xen_OPR_POW,
  Xen_OPR_MUL,
  Xen_OPR_DIV,
  Xen_OPR_MOD,
  Xen_OPR_ADD,
  Xen_OPR_SUB,
  Xen_OPR_LT,
  Xen_OPR_LE,
  Xen_OPR_EQ,
  Xen_OPR_GT,
  Xen_OPR_GE,
  Xen_OPR_NE,
  Xen_OPR_END,
} Xen_Opr;

Xen_Instance* Xen_Operator_Eval_Pair(Xen_Instance*, Xen_Instance*, Xen_Opr);
Xen_Instance* Xen_Operator_Eval_Pair_Steal2(Xen_Instance*, Xen_Instance*,
                                            Xen_Opr);

#endif
