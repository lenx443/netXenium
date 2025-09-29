#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "vm.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_vector.h"

Xen_Instance *Xen_Operator_Eval_Pair(Xen_Instance *first, Xen_Instance *second,
                                     Xen_Opr op) {
  if (op >= Xen_OPR_END) { return nil; }
  Xen_Instance *args = Xen_Vector_From_Array(1, &second);
  if (!args) { return nil; }
  if (!vm_run_callable(Xen_TYPE(first)->__opr[op], vm_root_ctx(), first, args)) {
    Xen_DEL_REF(args);
    return nil;
  }
  Xen_DEL_REF(args);
  Xen_Instance *result = xen_register_prop_get("__expose_opr", 0);
  if_nil_eval(result) { return nil; }
  return result;
}

Xen_Instance *Xen_Operator_Eval_Pair_Steal2(Xen_Instance *first, Xen_Instance *second,
                                            Xen_Opr op) {
  Xen_Instance *result = Xen_Operator_Eval_Pair(first, second, op);
  if_nil_eval(result) {
    Xen_DEL_REF(second);
    return nil;
  }
  Xen_DEL_REF(second);
  return result;
}
