#include "instance.h"
#include "xen_map_implement.h"
#include "xen_nil.h"
#include <assert.h>

int main() {
  Xen_Instance *map = __instance_new(&Xen_Map_Implement, nil, 0);
  assert(map != NULL);
  Xen_DEL_REF(map);
  return 0;
}
