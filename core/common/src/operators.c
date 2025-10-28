#include "operators.h"
#include "attrs.h"
#include "instance.h"
#include "xen_method.h"
#include "xen_vector.h"

static const char* Operators_Map[Xen_OPR_END] = {
    [Xen_OPR_EQ] = "__eq",   [Xen_OPR_GET_INDEX] = "__get_index",
    [Xen_OPR_POW] = "__pow", [Xen_OPR_MUL] = "__mul",
    [Xen_OPR_DIV] = "__div",
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
    Xen_DEL_REF(method);
    return NULL;
  }
  Xen_Instance* result = Xen_Method_Call(method, args);
  if (!result) {
    Xen_DEL_REF(args);
    Xen_DEL_REF(method);
    return NULL;
  }
  Xen_DEL_REF(args);
  Xen_DEL_REF(method);
  return result;
}

Xen_Instance* Xen_Operator_Eval_Pair_Steal2(Xen_Instance* first,
                                            Xen_Instance* second, Xen_Opr op) {
  Xen_Instance* result = Xen_Operator_Eval_Pair(first, second, op);
  if (!result) {
    Xen_DEL_REF(second);
    return NULL;
  }
  Xen_DEL_REF(second);
  return result;
}
