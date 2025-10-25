#include "operators.h"
#include "implement.h"
#include "instance.h"
#include "vm.h"
#include "xen_nil.h"
#include "xen_vector.h"

Xen_Instance* Xen_Operator_Eval_Pair(Xen_Instance* first, Xen_Instance* second,
                                     Xen_Opr op) {
  if (op >= Xen_OPR_END) {
    return NULL;
  }
  if (Xen_TYPE(first)->__opr[op] == NULL) {
    return NULL;
  }
  Xen_Instance* args = Xen_Vector_From_Array(1, &second);
  if (!args) {
    return NULL;
  }
  Xen_Instance* result =
      vm_run_callable(Xen_TYPE(first)->__opr[op], vm_root_ctx(), first, args);
  if (!result || Xen_Nil_Eval(result)) {
    Xen_DEL_REF(args);
    return NULL;
  }
  Xen_DEL_REF(args);
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
