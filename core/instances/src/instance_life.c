#include "instance_life.h"
#include "xen_number_implement.h"

int Xen_Instance_Init() {
  if (!Xen_Number_Init()) { return 0; }
  return 1;
}

void Xen_Instanse_Finish() { Xen_Number_Finish(); }
