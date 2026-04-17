#ifndef __OPERATORS_H__
#define __OPERATORS_H__

#include "instance.h"

typedef enum {
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
  Xen_OPR_HAS,
  Xen_OPR_BAND,
  Xen_OPR_BXOR,
  Xen_OPR_BOR,
  Xen_OPR_SHL,
  Xen_OPR_SHR,
  Xen_OPR_ASSIGN_POW,
  Xen_OPR_ASSIGN_MUL,
  Xen_OPR_ASSIGN_DIV,
  Xen_OPR_ASSIGN_MOD,
  Xen_OPR_ASSIGN_ADD,
  Xen_OPR_ASSIGN_SUB,
  Xen_OPR_ASSIGN_BAND,
  Xen_OPR_ASSIGN_BXOR,
  Xen_OPR_ASSIGN_BOR,
  Xen_OPR_ASSIGN_SHL,
  Xen_OPR_ASSIGN_SHR,
  Xen_OPR_END,
} Xen_Opr;

struct Xen_Operator_Info {
  const char *keyword;
  const char *op;
  Xen_Opr dfault;
};

extern struct Xen_Operator_Info Xen_Operators_Map[Xen_OPR_END];
Xen_Instance* Xen_Operator_Eval_Pair(Xen_Instance*, Xen_Instance*, Xen_Opr);

#endif
