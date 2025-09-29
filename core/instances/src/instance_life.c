#include "instance_life.h"
#include "xen_number_implement.h"
#include "xen_string_implement.h"
#include "xen_tuple_implement.h"
#include "xen_vector_implement.h"

int Xen_Instance_Init() {
  if (!Xen_Number_Init()) { return 0; }
  if (!Xen_String_Init()) {
    Xen_Number_Finish();
    return 0;
  }
  if (!Xen_Vector_Init()) {
    Xen_String_Finish();
    Xen_Number_Finish();
    return 0;
  }
  if (!Xen_Tuple_Init()) {
    Xen_Vector_Finish();
    Xen_String_Finish();
    Xen_Number_Finish();
  }
  return 1;
}

void Xen_Instanse_Finish() {
  Xen_Tuple_Finish();
  Xen_Vector_Finish();
  Xen_String_Finish();
  Xen_Number_Finish();
}
