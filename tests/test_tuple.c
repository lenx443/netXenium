#include <assert.h>
#include <string.h>

#include "attrs.h"
#include "instance.h"
#include "xen_life.h"
#include "xen_nil.h"
#include "xen_number.h"
#include "xen_string.h"
#include "xen_tuple.h"

int main(int argc, char** argv) {
  assert(Xen_Init(argc, argv));
  WITH_INSTANCE(foo, Xen_Number_From_Int(537))
  WITH_INSTANCE(bar, Xen_String_From_CString("Hello, World!"))
  WITH_INSTANCE(tuple,
                Xen_Tuple_From_Array(2, (Xen_Instance*[]){foo, bar, NULL})) {
    assert(Xen_Nil_NEval(tuple));
    WITH_INSTANCE(index0, Xen_Attr_Index_Size_Get(tuple, 0))
    WITH_INSTANCE(index1, Xen_Attr_Index_Size_Get(tuple, 1)) {
      assert(index0);
      assert(index1);
      assert(Xen_Number_As_Int(index0) == 537);
      assert(strcmp(Xen_String_As_CString(index1), "Hello, World!") == 0);
    }
  }
  Xen_Finish();
  return 0;
}
