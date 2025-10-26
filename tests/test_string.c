#include <assert.h>
#include <string.h>

#include "instance.h"
#include "operators.h"
#include "xen_life.h"
#include "xen_number.h"
#include "xen_string.h"

int main(int argc, char** argv) {
  assert(Xen_Init(argc, argv));
  Xen_Instance* string = Xen_String_From_CString("Test");
  assert(strcmp(Xen_String_As_CString(string), "Test") == 0);
  assert(Xen_Operator_Eval_Pair_Steal2(string, Xen_Number_From_Int(0),
                                       Xen_OPR_GET_INDEX) != NULL);
  Xen_DEL_REF(string);
  Xen_Finish();
  return 0;
}
