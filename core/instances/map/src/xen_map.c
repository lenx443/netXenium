#include "implement.h"
#include "instance.h"
#include "xen_map.h"
#include "xen_map_instance.h"

int Xen_Map_Push_Pair(Xen_Instance *map_inst, Xen_Map_Pair pair) {
  if (!pair.key || !pair.value || !Xen_TYPE(pair.key)->__hash) { return 0; }
  Xen_Map *map = (Xen_Map *)map_inst;
  return 1;
}
