#include "operators.h"
#include "attrs.h"
#include "instance.h"
#include "xen_method.h"
#include "xen_nil.h"
#include "xen_vector.h"

static const char* Operators_Map[Xen_OPR_END] = {
    [Xen_OPR_POW] = "__pow", [Xen_OPR_MUL] = "__mul", [Xen_OPR_DIV] = "__div",
    [Xen_OPR_MOD] = "__mod", [Xen_OPR_ADD] = "__add", [Xen_OPR_SUB] = "__sub",
    [Xen_OPR_LT] = "__lt",   [Xen_OPR_LE] = "__le",   [Xen_OPR_EQ] = "__eq",
    [Xen_OPR_GT] = "__gt",   [Xen_OPR_GE] = "__ge",   [Xen_OPR_NE] = "__ne",
    [Xen_OPR_HAS] = "__has",
};

Xen_Instance* Xen_Operator_Eval_Pair(Xen_Instance* first, Xen_Instance* second,
                                     Xen_Opr op) {
  if (op >= Xen_OPR_END) {
    return NULL;
  }
  Xen_Instance* method = Xen_Attr_Get_Str(first, Operators_Map[op]);
  if (!method) {
    return NULL;
  }
  Xen_Instance* args = Xen_Vector_From_Array(1, &second);
  if (!args) {
    return NULL;
  }
  Xen_Instance* result = Xen_Method_Call(method, args, nil);
  if (!result) {
    return NULL;
  }
  return result;
}
