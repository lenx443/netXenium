#include <assert.h>

#include "instance.h"
#include "operators.h"
#include "xen_life.h"
#include "xen_number.h"
#include "xen_vector.h"

int main(int argc, char **argv) {
  assert(Xen_Init(argc, argv));
  WITH_INSTANCE(foo, Xen_Number_From_Int(16))
  WITH_INSTANCE(bar, Xen_Number_From_Int(40))
  WITH_INSTANCE(vec, Xen_Vector_From_Array(2, (Xen_Instance *[]){foo, bar}))
  WITH_INSTANCE(index0, Xen_Operator_Eval_Pair_Steal2(vec, Xen_Number_From_Int(0),
                                                      Xen_OPR_GET_INDEX)) {
    assert(Xen_Number_As_Int(index0) == 16);
    WITH_INSTANCE(index1, Xen_Operator_Eval_Pair_Steal2(vec, Xen_Number_From_Int(1),
                                                        Xen_OPR_GET_INDEX)) {
      assert(Xen_Number_As_Int(index1) == 40);
    }
  }
  Xen_Finish();
  return 0;
}
