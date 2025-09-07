#include "implement.h"
#include "instance.h"
#include "operators.h"
#include "vm.h"
#include "xen_boolean.h"
#include "xen_boolean_implement.h"
#include "xen_nil.h"
#include "xen_register.h"
#include "xen_vector.h"

Xen_Instance *Xen_Operator_Eval_Pair(Xen_Instance *first, Xen_Instance *second,
                                     Xen_Opr op) {
  switch (op) {
  case Xen_EQ: {
    if (first == second) return Xen_True;
    Xen_Instance *args = Xen_Vector_From_Array_With_Size(1, &second);
    if (!args) { return nil; }
    if (!vm_run_callable(Xen_TYPE(first)->__opr.__eq, vm_root_ctx(), first, args)) {
      Xen_DEL_REF(args);
      return nil;
    }
    Xen_DEL_REF(args);
    Xen_Instance *result = xen_register_prop_get("__expose_opr_eq", 0);
    if (!result) { return nil; }
    if (Xen_TYPE(result) != &Xen_Boolean_Implement) { return nil; }
    if (result == Xen_True)
      return Xen_True;
    else if (result == Xen_False)
      return Xen_False;
    else
      return nil;
  }
  }
  return nil;
}
