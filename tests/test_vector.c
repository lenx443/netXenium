#include <assert.h>

#include "instance.h"
#include "xen_life.h"
#include "xen_number.h"
#include "xen_vector.h"

int main(int argc, char** argv) {
  assert(Xen_Init(argc, argv));
  Xen_Instance* foo = Xen_Number_From_Int(16);
  Xen_Instance* bar = Xen_Number_From_Int(40);
  Xen_Instance* vec = Xen_Vector_From_Array(2, (Xen_Instance*[]){foo, bar});
  Xen_Instance* index0 = Xen_Vector_Get_Index(vec, 0);
  assert(Xen_Number_As_Int(index0) == 16);
  Xen_Instance* index1 = Xen_Vector_Get_Index(vec, 1);
  assert(Xen_Number_As_Int(index1) == 40);
  Xen_Finish();
  return 0;
}
